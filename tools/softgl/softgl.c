/*
 * softgl — head-less software rasteriser for the OpenGL 1.1 subset used by
 * Dream Village.  Implements the fixed-function immediate-mode pipeline:
 *
 *   object coords --[modelview]--> eye --[projection]--> clip --/w--> NDC
 *   --[viewport]--> window coords --> triangle / line rasterisation
 *   (pixel-centre sampling, top-left fill rule, Gouraud colour interpolation)
 *   --> RGB8 framebuffer (bottom row first, like glReadPixels).
 *
 * Deliberately simple and deterministic: no depth buffer, no blending, no
 * anti-aliasing, no textures — none of which the scene uses.  Every GL error
 * that a conforming implementation would raise for the calls we support is
 * recorded, so the harness can report them (the original program raises
 * GL_INVALID_OPERATION hundreds of times per frame).
 */
#include <GL/gl.h>
#include <GL/glut.h>
#include "softgl.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ----------------------------------------------------------------------- */
/* state                                                                    */
/* ----------------------------------------------------------------------- */

#define STACK_DEPTH 32
#define MAX_VERTS   65536

typedef struct { double m[16]; } mat4;              /* column-major, as in GL */

static mat4   mv_stack[STACK_DEPTH], pr_stack[STACK_DEPTH];
static int    mv_top, pr_top;
static GLenum matrix_mode = GL_MODELVIEW;
static mat4   pm_cache;                              /* projection * modelview */
static int    pm_valid;

static unsigned char* fb;                            /* RGB8, bottom row first */
static int   fbw, fbh;
static int   vp_x, vp_y, vp_w, vp_h;
static float clear_color[4];
static float cur_color[3] = {1.f, 1.f, 1.f};
static float line_width = 1.f;
static int   pack_alignment = 4;
static GLenum first_error = GL_NO_ERROR;
static softgl_stats stats;

typedef struct { double x, y; float c[3]; } vtx;     /* window coords + colour */
static vtx*   verts;
static int    nverts;
static int    in_begin;
static GLenum prim_mode;
static int    initialised;

static void set_error(GLenum e)
{
    if (first_error == GL_NO_ERROR) first_error = e;
    stats.errors++;
}

static void mat_identity(mat4* a)
{
    memset(a, 0, sizeof *a);
    a->m[0] = a->m[5] = a->m[10] = a->m[15] = 1.0;
}

static mat4* cur_matrix(void)
{
    return matrix_mode == GL_PROJECTION ? &pr_stack[pr_top] : &mv_stack[mv_top];
}

/* a = a * b  (post-multiplication, exactly what glMultMatrix does) */
static void mat_mul(mat4* a, const mat4* b)
{
    mat4 r;
    int i, j, k;
    for (i = 0; i < 4; ++i)
        for (j = 0; j < 4; ++j) {
            double s = 0.0;
            for (k = 0; k < 4; ++k) s += a->m[k * 4 + i] * b->m[j * 4 + k];
            r.m[j * 4 + i] = s;
        }
    *a = r;
}

static void ensure_init(void)
{
    if (initialised) return;
    initialised = 1;
    mat_identity(&mv_stack[0]);
    mat_identity(&pr_stack[0]);
    verts = (vtx*)malloc(sizeof(vtx) * MAX_VERTS);
    if (!fb) softgl_create_framebuffer(1300, 650);
}

/* ----------------------------------------------------------------------- */
/* extension API                                                            */
/* ----------------------------------------------------------------------- */

void softgl_create_framebuffer(int width, int height)
{
    free(fb);
    fbw = width;
    fbh = height;
    fb = (unsigned char*)calloc((size_t)fbw * (size_t)fbh * 3u, 1u);
    vp_x = vp_y = 0;
    vp_w = fbw;
    vp_h = fbh;
    ensure_init();
}

const unsigned char* softgl_framebuffer(int* width, int* height)
{
    ensure_init();
    if (width) *width = fbw;
    if (height) *height = fbh;
    return fb;
}

void softgl_get_stats(softgl_stats* out) { *out = stats; }
void softgl_reset_stats(void) { memset(&stats, 0, sizeof stats); }

/* ----------------------------------------------------------------------- */
/* rasterisation                                                            */
/* ----------------------------------------------------------------------- */

static void put_pixel(int x, int y, const float* c)
{
    unsigned char* p;
    int i;
    if (x < vp_x || y < vp_y || x >= vp_x + vp_w || y >= vp_y + vp_h) return;
    if (x < 0 || y < 0 || x >= fbw || y >= fbh) return;
    p = fb + ((size_t)y * (size_t)fbw + (size_t)x) * 3u;
    for (i = 0; i < 3; ++i) {
        float v = c[i];
        if (v < 0.f) v = 0.f;
        if (v > 1.f) v = 1.f;
        p[i] = (unsigned char)(v * 255.f + 0.5f);
    }
    stats.fragments++;
}

/* Does the directed edge a->b of a counter-clockwise triangle own the pixels
 * whose centres lie exactly on it?  (The "top-left" rule expressed for a
 * y-up window space: left edges and bottom edges own their boundary.) */
static int owns_edge(const vtx* a, const vtx* b)
{
    double dx = b->x - a->x, dy = b->y - a->y;
    return dy < 0.0 || (dy == 0.0 && dx > 0.0);
}

static void raster_triangle(const vtx* v0, const vtx* v1, const vtx* v2)
{
    const vtx* t;
    double area, minx, maxx, miny, maxy;
    int x0, x1, y0, y1, x, y, own0, own1, own2;

    area = (v1->x - v0->x) * (v2->y - v0->y) - (v2->x - v0->x) * (v1->y - v0->y);
    if (area == 0.0) return;
    if (area < 0.0) { t = v1; v1 = v2; v2 = t; area = -area; }   /* make CCW */
    stats.triangles++;

    minx = fmin(v0->x, fmin(v1->x, v2->x)); maxx = fmax(v0->x, fmax(v1->x, v2->x));
    miny = fmin(v0->y, fmin(v1->y, v2->y)); maxy = fmax(v0->y, fmax(v1->y, v2->y));
    x0 = (int)floor(minx); x1 = (int)ceil(maxx);
    y0 = (int)floor(miny); y1 = (int)ceil(maxy);
    if (x0 < vp_x) x0 = vp_x;
    if (y0 < vp_y) y0 = vp_y;
    if (x1 > vp_x + vp_w) x1 = vp_x + vp_w;
    if (y1 > vp_y + vp_h) y1 = vp_y + vp_h;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > fbw) x1 = fbw;
    if (y1 > fbh) y1 = fbh;

    own0 = owns_edge(v1, v2);   /* edge opposite v0 */
    own1 = owns_edge(v2, v0);   /* edge opposite v1 */
    own2 = owns_edge(v0, v1);   /* edge opposite v2 */

    for (y = y0; y < y1; ++y) {
        double py = y + 0.5;
        for (x = x0; x < x1; ++x) {
            double px = x + 0.5;
            double e0 = (v2->x - v1->x) * (py - v1->y) - (v2->y - v1->y) * (px - v1->x);
            double e1 = (v0->x - v2->x) * (py - v2->y) - (v0->y - v2->y) * (px - v2->x);
            double e2 = (v1->x - v0->x) * (py - v0->y) - (v1->y - v0->y) * (px - v0->x);
            float c[3];
            double l0, l1, l2;
            int i;
            if (e0 < 0.0 || e1 < 0.0 || e2 < 0.0) continue;
            if ((e0 == 0.0 && !own0) || (e1 == 0.0 && !own1) || (e2 == 0.0 && !own2)) continue;
            l0 = e0 / area; l1 = e1 / area; l2 = e2 / area;
            for (i = 0; i < 3; ++i)
                c[i] = (float)(l0 * v0->c[i] + l1 * v1->c[i] + l2 * v2->c[i]);
            put_pixel(x, y, c);
        }
    }
}

/* Width-1 line: diamond-exit approximation by DDA along the major axis; the
 * end point is excluded (half-open), as in GL. */
static void raster_line(const vtx* a, const vtx* b)
{
    double dx = b->x - a->x, dy = b->y - a->y;
    stats.lines++;

    if (line_width > 1.f) {                    /* wide line -> rectangle */
        double len = sqrt(dx * dx + dy * dy), nx, ny;
        vtx q[4];
        if (len == 0.0) return;
        nx = -dy / len * line_width * 0.5;
        ny =  dx / len * line_width * 0.5;
        q[0] = *a; q[0].x += nx; q[0].y += ny;
        q[1] = *b; q[1].x += nx; q[1].y += ny;
        q[2] = *b; q[2].x -= nx; q[2].y -= ny;
        q[3] = *a; q[3].x -= nx; q[3].y -= ny;
        raster_triangle(&q[0], &q[1], &q[2]);
        raster_triangle(&q[0], &q[2], &q[3]);
        return;
    }

    if (fabs(dx) >= fabs(dy)) {
        int xs = (int)floor(a->x), xe = (int)floor(b->x), step = dx >= 0 ? 1 : -1, x;
        if (dx == 0.0) return;
        for (x = xs; x != xe; x += step) {
            double t = ((x + 0.5) - a->x) / dx;
            double y = a->y + t * dy;
            float c[3];
            int i;
            if (t < 0.0 || t > 1.0) continue;
            for (i = 0; i < 3; ++i) c[i] = (float)(a->c[i] + t * (b->c[i] - a->c[i]));
            put_pixel(x, (int)floor(y), c);
        }
    } else {
        int ys = (int)floor(a->y), ye = (int)floor(b->y), step = dy >= 0 ? 1 : -1, y;
        for (y = ys; y != ye; y += step) {
            double t = ((y + 0.5) - a->y) / dy;
            double x = a->x + t * dx;
            float c[3];
            int i;
            if (t < 0.0 || t > 1.0) continue;
            for (i = 0; i < 3; ++i) c[i] = (float)(a->c[i] + t * (b->c[i] - a->c[i]));
            put_pixel((int)floor(x), y, c);
        }
    }
}

static void flush_primitive(void)
{
    int i;
    switch (prim_mode) {
    case GL_POINTS:
        for (i = 0; i < nverts; ++i) put_pixel((int)floor(verts[i].x), (int)floor(verts[i].y), verts[i].c);
        break;
    case GL_LINES:
        for (i = 0; i + 1 < nverts; i += 2) raster_line(&verts[i], &verts[i + 1]);
        break;
    case GL_LINE_STRIP:
        for (i = 0; i + 1 < nverts; ++i) raster_line(&verts[i], &verts[i + 1]);
        break;
    case GL_LINE_LOOP:
        for (i = 0; i + 1 < nverts; ++i) raster_line(&verts[i], &verts[i + 1]);
        if (nverts > 2) raster_line(&verts[nverts - 1], &verts[0]);
        break;
    case GL_TRIANGLES:
        for (i = 0; i + 2 < nverts; i += 3) raster_triangle(&verts[i], &verts[i + 1], &verts[i + 2]);
        break;
    case GL_TRIANGLE_STRIP:
        for (i = 0; i + 2 < nverts; ++i)
            if (i & 1) raster_triangle(&verts[i + 1], &verts[i], &verts[i + 2]);
            else       raster_triangle(&verts[i], &verts[i + 1], &verts[i + 2]);
        break;
    case GL_TRIANGLE_FAN:
    case GL_POLYGON:
        for (i = 1; i + 1 < nverts; ++i) raster_triangle(&verts[0], &verts[i], &verts[i + 1]);
        break;
    case GL_QUADS:
        for (i = 0; i + 3 < nverts; i += 4) {
            raster_triangle(&verts[i], &verts[i + 1], &verts[i + 2]);
            raster_triangle(&verts[i], &verts[i + 2], &verts[i + 3]);
        }
        break;
    case GL_QUAD_STRIP:
        for (i = 0; i + 3 < nverts; i += 2) {
            raster_triangle(&verts[i], &verts[i + 1], &verts[i + 3]);
            raster_triangle(&verts[i], &verts[i + 3], &verts[i + 2]);
        }
        break;
    default:
        break;
    }
}

/* ----------------------------------------------------------------------- */
/* GL entry points                                                          */
/* ----------------------------------------------------------------------- */

#define REQUIRE_OUTSIDE_BEGIN() \
    do { ensure_init(); if (in_begin) { set_error(GL_INVALID_OPERATION); return; } } while (0)

void glBegin(GLenum mode)
{
    REQUIRE_OUTSIDE_BEGIN();
    if (mode > GL_POLYGON) { set_error(GL_INVALID_ENUM); return; }
    in_begin = 1;
    prim_mode = mode;
    nverts = 0;
    stats.begin_end_pairs++;
}

void glEnd(void)
{
    ensure_init();
    if (!in_begin) { set_error(GL_INVALID_OPERATION); return; }
    flush_primitive();
    in_begin = 0;
    nverts = 0;
}

static void emit_vertex(double x, double y, double z)
{
    /* Transform with the combined projection*modelview matrix, as real
     * implementations do.  (Transforming by modelview and projection in two
     * steps gives results that differ in the last bit depending on which of
     * the two matrices holds the ortho transform — enough to flip pixels whose
     * centres lie exactly on an edge.) */
    const double* pm;
    double c[4], w;
    vtx* v;
    int i;
    if (!in_begin) { set_error(GL_INVALID_OPERATION); return; }
    if (nverts >= MAX_VERTS) { set_error(GL_OUT_OF_MEMORY); return; }
    if (!pm_valid) { pm_cache = pr_stack[pr_top]; mat_mul(&pm_cache, &mv_stack[mv_top]); pm_valid = 1; }
    pm = pm_cache.m;
    for (i = 0; i < 4; ++i) c[i] = pm[i] * x + pm[4 + i] * y + pm[8 + i] * z + pm[12 + i];
    w = c[3] != 0.0 ? c[3] : 1.0;
    v = &verts[nverts++];
    v->x = vp_x + (c[0] / w + 1.0) * 0.5 * vp_w;
    v->y = vp_y + (c[1] / w + 1.0) * 0.5 * vp_h;
    memcpy(v->c, cur_color, sizeof v->c);
    stats.vertices++;
}

void glVertex2d(GLdouble x, GLdouble y)               { ensure_init(); emit_vertex(x, y, 0.0); }
void glVertex2f(GLfloat x, GLfloat y)                 { ensure_init(); emit_vertex(x, y, 0.0); }
void glVertex2i(GLint x, GLint y)                     { ensure_init(); emit_vertex(x, y, 0.0); }
void glVertex3d(GLdouble x, GLdouble y, GLdouble z)   { ensure_init(); emit_vertex(x, y, z); }
void glVertex3f(GLfloat x, GLfloat y, GLfloat z)      { ensure_init(); emit_vertex(x, y, z); }

void glColor3ub(GLubyte r, GLubyte g, GLubyte b)
{
    ensure_init();
    cur_color[0] = r / 255.f; cur_color[1] = g / 255.f; cur_color[2] = b / 255.f;
}
void glColor3f(GLfloat r, GLfloat g, GLfloat b)
{
    ensure_init();
    cur_color[0] = r; cur_color[1] = g; cur_color[2] = b;
}
void glColor3d(GLdouble r, GLdouble g, GLdouble b)    { glColor3f((GLfloat)r, (GLfloat)g, (GLfloat)b); }
void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { (void)a; glColor3f(r, g, b); }

void glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    REQUIRE_OUTSIDE_BEGIN();
    glBegin(GL_POLYGON);
    glVertex2f(x1, y1); glVertex2f(x2, y1); glVertex2f(x2, y2); glVertex2f(x1, y2);
    glEnd();
}
void glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
    glRectf((GLfloat)x1, (GLfloat)y1, (GLfloat)x2, (GLfloat)y2);
}

void glMatrixMode(GLenum mode)
{
    REQUIRE_OUTSIDE_BEGIN();
    if (mode != GL_MODELVIEW && mode != GL_PROJECTION) { set_error(GL_INVALID_ENUM); return; }
    matrix_mode = mode;
}
void glLoadIdentity(void) { REQUIRE_OUTSIDE_BEGIN(); mat_identity(cur_matrix()); pm_valid = 0; }

void glPushMatrix(void)
{
    int* top = matrix_mode == GL_PROJECTION ? &pr_top : &mv_top;
    mat4* st = matrix_mode == GL_PROJECTION ? pr_stack : mv_stack;
    REQUIRE_OUTSIDE_BEGIN();
    if (*top + 1 >= STACK_DEPTH) { set_error(GL_STACK_OVERFLOW); return; }
    st[*top + 1] = st[*top];
    (*top)++;
    pm_valid = 0;
}
void glPopMatrix(void)
{
    int* top = matrix_mode == GL_PROJECTION ? &pr_top : &mv_top;
    REQUIRE_OUTSIDE_BEGIN();
    if (*top == 0) { set_error(GL_STACK_UNDERFLOW); return; }
    (*top)--;
    pm_valid = 0;
}

void glTranslated(GLdouble x, GLdouble y, GLdouble z)
{
    mat4 t;
    REQUIRE_OUTSIDE_BEGIN();
    mat_identity(&t);
    t.m[12] = x; t.m[13] = y; t.m[14] = z;
    mat_mul(cur_matrix(), &t);
    pm_valid = 0;
}
void glTranslatef(GLfloat x, GLfloat y, GLfloat z) { glTranslated(x, y, z); }

void glScaled(GLdouble x, GLdouble y, GLdouble z)
{
    mat4 s;
    REQUIRE_OUTSIDE_BEGIN();
    mat_identity(&s);
    s.m[0] = x; s.m[5] = y; s.m[10] = z;
    mat_mul(cur_matrix(), &s);
    pm_valid = 0;
}
void glScalef(GLfloat x, GLfloat y, GLfloat z) { glScaled(x, y, z); }

void glOrtho(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f)
{
    mat4 o;
    REQUIRE_OUTSIDE_BEGIN();
    if (l == r || b == t || n == f) { set_error(GL_INVALID_VALUE); return; }
    mat_identity(&o);
    o.m[0]  =  2.0 / (r - l);
    o.m[5]  =  2.0 / (t - b);
    o.m[10] = -2.0 / (f - n);
    o.m[12] = -(r + l) / (r - l);
    o.m[13] = -(t + b) / (t - b);
    o.m[14] = -(f + n) / (f - n);
    mat_mul(cur_matrix(), &o);
    pm_valid = 0;
}

void glViewport(GLint x, GLint y, GLsizei w, GLsizei h)
{
    REQUIRE_OUTSIDE_BEGIN();
    if (w < 0 || h < 0) { set_error(GL_INVALID_VALUE); return; }
    vp_x = x; vp_y = y; vp_w = w; vp_h = h;
}

void glClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a)
{
    REQUIRE_OUTSIDE_BEGIN();
    clear_color[0] = r < 0 ? 0 : r > 1 ? 1 : r;
    clear_color[1] = g < 0 ? 0 : g > 1 ? 1 : g;
    clear_color[2] = b < 0 ? 0 : b > 1 ? 1 : b;
    clear_color[3] = a < 0 ? 0 : a > 1 ? 1 : a;
}

void glClear(GLbitfield mask)
{
    int x, y;
    REQUIRE_OUTSIDE_BEGIN();
    if (mask & ~(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)) { set_error(GL_INVALID_VALUE); return; }
    if (!(mask & GL_COLOR_BUFFER_BIT)) return;
    for (y = vp_y; y < vp_y + vp_h; ++y) {
        if (y < 0 || y >= fbh) continue;
        for (x = vp_x; x < vp_x + vp_w; ++x) {
            unsigned char* p;
            int i;
            if (x < 0 || x >= fbw) continue;
            p = fb + ((size_t)y * (size_t)fbw + (size_t)x) * 3u;
            for (i = 0; i < 3; ++i) p[i] = (unsigned char)(clear_color[i] * 255.f + 0.5f);
        }
    }
}

void glFlush(void)  { REQUIRE_OUTSIDE_BEGIN(); }
void glFinish(void) { REQUIRE_OUTSIDE_BEGIN(); }

void glLineWidth(GLfloat width)
{
    REQUIRE_OUTSIDE_BEGIN();                    /* inside glBegin/glEnd: GL_INVALID_OPERATION */
    if (width <= 0.f) { set_error(GL_INVALID_VALUE); return; }
    line_width = width;
}

void glShadeModel(GLenum mode)
{
    REQUIRE_OUTSIDE_BEGIN();
    if (mode != GL_FLAT && mode != GL_SMOOTH) set_error(GL_INVALID_ENUM);
    /* flat shading is not implemented; the scene never requests it */
}

void glEnable(GLenum cap)  { REQUIRE_OUTSIDE_BEGIN(); (void)cap; }
void glDisable(GLenum cap) { REQUIRE_OUTSIDE_BEGIN(); (void)cap; }

void glPixelStorei(GLenum pname, GLint param)
{
    REQUIRE_OUTSIDE_BEGIN();
    if (pname == GL_PACK_ALIGNMENT) {
        if (param == 1 || param == 2 || param == 4 || param == 8) pack_alignment = param;
        else set_error(GL_INVALID_VALUE);
    }
}

void glReadPixels(GLint x, GLint y, GLsizei w, GLsizei h, GLenum format, GLenum type, GLvoid* pixels)
{
    size_t stride;
    int row, col;
    REQUIRE_OUTSIDE_BEGIN();
    if (format != GL_RGB || type != GL_UNSIGNED_BYTE) { set_error(GL_INVALID_ENUM); return; }
    if (w < 0 || h < 0) { set_error(GL_INVALID_VALUE); return; }
    stride = ((size_t)w * 3u + (size_t)pack_alignment - 1u) / (size_t)pack_alignment * (size_t)pack_alignment;
    for (row = 0; row < h; ++row) {
        unsigned char* dst = (unsigned char*)pixels + (size_t)row * stride;
        int sy = y + row;
        for (col = 0; col < w; ++col) {
            int sx = x + col;
            if (sx < 0 || sy < 0 || sx >= fbw || sy >= fbh) { dst[col * 3] = dst[col * 3 + 1] = dst[col * 3 + 2] = 0; continue; }
            memcpy(dst + col * 3, fb + ((size_t)sy * (size_t)fbw + (size_t)sx) * 3u, 3u);
        }
    }
}

void glGetIntegerv(GLenum pname, GLint* params)
{
    REQUIRE_OUTSIDE_BEGIN();
    switch (pname) {
    case GL_VIEWPORT: params[0] = vp_x; params[1] = vp_y; params[2] = vp_w; params[3] = vp_h; break;
    case GL_PACK_ALIGNMENT: params[0] = pack_alignment; break;
    case GL_MAX_MODELVIEW_STACK_DEPTH:
    case GL_MAX_PROJECTION_STACK_DEPTH: params[0] = STACK_DEPTH; break;
    default: set_error(GL_INVALID_ENUM); break;
    }
}

GLenum glGetError(void)
{
    GLenum e;
    ensure_init();
    if (in_begin) { set_error(GL_INVALID_OPERATION); return GL_INVALID_OPERATION; }
    e = first_error;
    first_error = GL_NO_ERROR;
    return e;
}

const GLubyte* glGetString(GLenum name)
{
    ensure_init();
    switch (name) {
    case GL_VENDOR:   return (const GLubyte*)"softgl";
    case GL_RENDERER: return (const GLubyte*)"softgl software rasteriser";
    case GL_VERSION:  return (const GLubyte*)"1.1 softgl";
    case GL_EXTENSIONS: return (const GLubyte*)"";
    default: set_error(GL_INVALID_ENUM); return NULL;
    }
}

/* ----------------------------------------------------------------------- */
/* GLUT stand-ins: enough for the legacy program to link; all no-ops        */
/* ----------------------------------------------------------------------- */

void glutInit(int* argc, char** argv)                     { (void)argc; (void)argv; ensure_init(); }
void glutInitDisplayMode(unsigned int mode)               { (void)mode; }
void glutInitWindowSize(int w, int h)                     { (void)w; (void)h; }
void glutInitWindowPosition(int x, int y)                 { (void)x; (void)y; }
int  glutCreateWindow(const char* title)                  { (void)title; ensure_init(); return 1; }
void glutDisplayFunc(void (*f)(void))                     { (void)f; }
void glutIdleFunc(void (*f)(void))                        { (void)f; }
void glutKeyboardFunc(void (*f)(unsigned char, int, int)) { (void)f; }
void glutMouseFunc(void (*f)(int, int, int, int))         { (void)f; }
void glutReshapeFunc(void (*f)(int, int))                 { (void)f; }
void glutTimerFunc(unsigned int ms, void (*f)(int), int v){ (void)ms; (void)f; (void)v; }
void glutMainLoop(void)                                   { }
void glutSwapBuffers(void)                                { }
void glutPostRedisplay(void)                              { stats.post_redisplay++; }
