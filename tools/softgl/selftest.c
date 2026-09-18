/* Self-test for the softgl rasteriser: pixel-exact expectations for the
 * primitives and state the scene relies on.  Exit status 0 = all pass. */
#include <GL/gl.h>
#include "softgl.h"

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

static const unsigned char* px(int x, int y)   /* y counted from the bottom, like GL */
{
    int w, h;
    const unsigned char* fb = softgl_framebuffer(&w, &h);
    return fb + ((size_t)y * (size_t)w + (size_t)x) * 3u;
}
static int is(int x, int y, int r, int g, int b)
{
    const unsigned char* p = px(x, y);
    return p[0] == r && p[1] == g && p[2] == b;
}
static unsigned long count_color(int r, int g, int b)
{
    int w, h, x, y;
    unsigned long n = 0;
    softgl_framebuffer(&w, &h);
    for (y = 0; y < h; ++y)
        for (x = 0; x < w; ++x)
            if (is(x, y, r, g, b)) ++n;
    return n;
}

static void setup(void)
{
    softgl_create_framebuffer(100, 50);            /* 1 world unit = 1 pixel in x, 0.5 in y */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 100, 0, 50, -1, 1);                 /* 1:1 mapping in both axes */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    softgl_reset_stats();
    (void)glGetError();
}

static void test_clear_and_rect(void)
{
    setup();
    glColor3ub(10, 20, 30);
    glRectf(10, 10, 20, 15);                       /* 10x5 pixels, edges on pixel boundaries */
    CHECK(count_color(10, 20, 30) == 50);
    CHECK(is(10, 10, 10, 20, 30));                 /* bottom-left corner pixel is inside */
    CHECK(is(19, 14, 10, 20, 30));
    CHECK(is(20, 10, 0, 0, 0));                    /* right edge exclusive */
    CHECK(is(10, 15, 0, 0, 0));                    /* top edge exclusive */
    CHECK(glGetError() == GL_NO_ERROR);
}

static void test_shared_edge_no_gap_no_overlap(void)
{
    /* Two triangles sharing a diagonal must cover a rectangle exactly once. */
    softgl_stats st;
    setup();
    glColor3f(1, 1, 1);
    glBegin(GL_TRIANGLES);
    glVertex2f(0, 0); glVertex2f(40, 0); glVertex2f(40, 30);
    glVertex2f(0, 0); glVertex2f(40, 30); glVertex2f(0, 30);
    glEnd();
    softgl_get_stats(&st);
    CHECK(count_color(255, 255, 255) == 40 * 30);
    CHECK(st.fragments == 40 * 30);                /* no pixel written twice */
    CHECK(st.triangles == 2);
}

static void test_quads_polygon_fan(void)
{
    setup();
    glColor3ub(255, 0, 0);
    glBegin(GL_QUADS);
    glVertex2f(0, 0); glVertex2f(10, 0); glVertex2f(10, 10); glVertex2f(0, 10);
    glEnd();
    glColor3ub(0, 255, 0);
    glBegin(GL_POLYGON);                           /* hexagon-ish: a 20x10 box with chamfers is still convex */
    glVertex2f(30, 0); glVertex2f(50, 0); glVertex2f(50, 10); glVertex2f(30, 10);
    glEnd();
    glColor3ub(0, 0, 255);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(70, 0); glVertex2f(80, 0); glVertex2f(80, 10); glVertex2f(70, 10);
    glEnd();
    CHECK(count_color(255, 0, 0) == 100);
    CHECK(count_color(0, 255, 0) == 200);
    CHECK(count_color(0, 0, 255) == 100);
}

static void test_gouraud(void)
{
    /* Colour must be interpolated: left edge red, right edge blue. */
    const unsigned char* p;
    setup();
    glBegin(GL_QUADS);
    glColor3f(1, 0, 0); glVertex2f(0, 0);  glVertex2f(0, 10);
    glColor3f(0, 0, 1); glVertex2f(100, 10); glVertex2f(100, 0);
    glEnd();
    p = px(0, 5);  CHECK(p[0] > 240 && p[2] < 15);
    p = px(99, 5); CHECK(p[2] > 240 && p[0] < 15);
    p = px(50, 5); CHECK(p[0] > 100 && p[0] < 155 && p[2] > 100 && p[2] < 155);
}

static void test_matrix_stack_and_translate(void)
{
    setup();
    glColor3ub(9, 9, 9);
    glPushMatrix();
    glTranslated(50, 20, 0);
    glRectf(0, 0, 5, 5);                           /* lands at 50..55 x 20..25 */
    glPopMatrix();
    glRectf(0, 0, 5, 5);                           /* back at the origin */
    CHECK(is(52, 22, 9, 9, 9));
    CHECK(is(2, 2, 9, 9, 9));
    CHECK(count_color(9, 9, 9) == 50);
    glPopMatrix();                                 /* underflow */
    CHECK(glGetError() == GL_STACK_UNDERFLOW);
}

static void test_lines(void)
{
    setup();
    glColor3f(1, 1, 1);
    glBegin(GL_LINES);
    glVertex2f(10.5f, 10.5f); glVertex2f(20.5f, 10.5f);   /* horizontal, 10 pixels, end excluded */
    glVertex2f(30.5f, 10.5f); glVertex2f(30.5f, 20.5f);   /* vertical */
    glEnd();
    CHECK(count_color(255, 255, 255) == 20);
    CHECK(is(10, 10, 255, 255, 255));
    CHECK(is(19, 10, 255, 255, 255));
    CHECK(is(20, 10, 0, 0, 0));
    CHECK(is(30, 19, 255, 255, 255));
}

static void test_errors_inside_begin(void)
{
    softgl_stats st;
    setup();
    glBegin(GL_LINES);
    glLineWidth(5);                                /* not allowed between glBegin/glEnd */
    glVertex2f(0, 0); glVertex2f(10, 0);
    glEnd();
    CHECK(glGetError() == GL_INVALID_OPERATION);
    softgl_get_stats(&st);
    CHECK(st.errors == 1);
    glBegin(GL_QUADS);
    glBegin(GL_QUADS);                             /* nested begin */
    glEnd();
    CHECK(glGetError() == GL_INVALID_OPERATION);
    glEnd();                                       /* unbalanced end */
    CHECK(glGetError() == GL_INVALID_OPERATION);
}

static void test_readpixels_roundtrip(void)
{
    unsigned char buf[3 * 4 * 2];
    setup();
    glColor3ub(1, 2, 3);
    glRectf(0, 0, 4, 2);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, 4, 2, GL_RGB, GL_UNSIGNED_BYTE, buf);
    CHECK(buf[0] == 1 && buf[1] == 2 && buf[2] == 3);
    CHECK(buf[3 * 7] == 1 && buf[3 * 7 + 2] == 3);
    CHECK(glGetError() == GL_NO_ERROR);
}

int main(void)
{
    test_clear_and_rect();
    test_shared_edge_no_gap_no_overlap();
    test_quads_polygon_fan();
    test_gouraud();
    test_matrix_stack_and_translate();
    test_lines();
    test_errors_inside_begin();
    test_readpixels_roundtrip();
    if (failures) { printf("%d check(s) failed\n", failures); return 1; }
    printf("softgl self-test: all checks passed\n");
    return 0;
}
