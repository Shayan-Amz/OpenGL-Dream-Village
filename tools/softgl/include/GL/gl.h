/*
 * softgl — a software implementation of the small fixed-function OpenGL 1.1
 * subset used by Dream Village (immediate-mode vertices, matrix stack, lines,
 * triangles, quads, polygons, framebuffer read-back).  It exists so that the
 * scene can be rendered head-less — without a GPU, a window system or a
 * vendor driver — for automated testing.  See tools/softgl/README.md.
 *
 * This header mirrors the names and enumerant values of <GL/gl.h> for the
 * functions that softgl implements, so that application code compiles
 * unchanged against either the real OpenGL or softgl.
 */
#ifndef SOFTGL_GL_H
#define SOFTGL_GL_H

#ifdef __cplusplus
extern "C" {
#endif

#define GLAPI
#define APIENTRY
#define GLAPIENTRY

typedef unsigned int   GLenum;
typedef unsigned char  GLboolean;
typedef unsigned int   GLbitfield;
typedef signed char    GLbyte;
typedef short          GLshort;
typedef int            GLint;
typedef int            GLsizei;
typedef unsigned char  GLubyte;
typedef unsigned short GLushort;
typedef unsigned int   GLuint;
typedef float          GLfloat;
typedef float          GLclampf;
typedef double         GLdouble;
typedef double         GLclampd;
typedef void           GLvoid;

#define GL_FALSE 0
#define GL_TRUE  1

/* primitive types */
#define GL_POINTS         0x0000
#define GL_LINES          0x0001
#define GL_LINE_LOOP      0x0002
#define GL_LINE_STRIP     0x0003
#define GL_TRIANGLES      0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRIANGLE_FAN   0x0006
#define GL_QUADS          0x0007
#define GL_QUAD_STRIP     0x0008
#define GL_POLYGON        0x0009

/* buffers, matrices, pixel formats */
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_MODELVIEW        0x1700
#define GL_PROJECTION       0x1701
#define GL_RGB              0x1907
#define GL_RGBA             0x1908
#define GL_UNSIGNED_BYTE    0x1401
#define GL_PACK_ALIGNMENT   0x0D05
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_FLAT             0x1D00
#define GL_SMOOTH           0x1D01
#define GL_VIEWPORT         0x0BA2
#define GL_LINE_WIDTH       0x0B21
#define GL_MAX_MODELVIEW_STACK_DEPTH  0x0D36
#define GL_MAX_PROJECTION_STACK_DEPTH 0x0D38

/* strings */
#define GL_VENDOR     0x1F00
#define GL_RENDERER   0x1F01
#define GL_VERSION    0x1F02
#define GL_EXTENSIONS 0x1F03

/* errors */
#define GL_NO_ERROR          0
#define GL_INVALID_ENUM      0x0500
#define GL_INVALID_VALUE     0x0501
#define GL_INVALID_OPERATION 0x0502
#define GL_STACK_OVERFLOW    0x0503
#define GL_STACK_UNDERFLOW   0x0504
#define GL_OUT_OF_MEMORY     0x0505

/* capabilities accepted (and ignored) by glEnable/glDisable */
#define GL_DEPTH_TEST  0x0B71
#define GL_BLEND       0x0BE2
#define GL_LINE_SMOOTH 0x0B20

GLAPI void APIENTRY glBegin(GLenum mode);
GLAPI void APIENTRY glEnd(void);
GLAPI void APIENTRY glVertex2d(GLdouble x, GLdouble y);
GLAPI void APIENTRY glVertex2f(GLfloat x, GLfloat y);
GLAPI void APIENTRY glVertex2i(GLint x, GLint y);
GLAPI void APIENTRY glVertex3d(GLdouble x, GLdouble y, GLdouble z);
GLAPI void APIENTRY glVertex3f(GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glColor3ub(GLubyte r, GLubyte g, GLubyte b);
GLAPI void APIENTRY glColor3f(GLfloat r, GLfloat g, GLfloat b);
GLAPI void APIENTRY glColor3d(GLdouble r, GLdouble g, GLdouble b);
GLAPI void APIENTRY glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
GLAPI void APIENTRY glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
GLAPI void APIENTRY glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);

GLAPI void APIENTRY glMatrixMode(GLenum mode);
GLAPI void APIENTRY glLoadIdentity(void);
GLAPI void APIENTRY glPushMatrix(void);
GLAPI void APIENTRY glPopMatrix(void);
GLAPI void APIENTRY glTranslated(GLdouble x, GLdouble y, GLdouble z);
GLAPI void APIENTRY glTranslatef(GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glScaled(GLdouble x, GLdouble y, GLdouble z);
GLAPI void APIENTRY glScalef(GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top,
                            GLdouble zNear, GLdouble zFar);
GLAPI void APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height);

GLAPI void APIENTRY glClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a);
GLAPI void APIENTRY glClear(GLbitfield mask);
GLAPI void APIENTRY glFlush(void);
GLAPI void APIENTRY glFinish(void);
GLAPI void APIENTRY glLineWidth(GLfloat width);
GLAPI void APIENTRY glShadeModel(GLenum mode);
GLAPI void APIENTRY glEnable(GLenum cap);
GLAPI void APIENTRY glDisable(GLenum cap);
GLAPI void APIENTRY glPixelStorei(GLenum pname, GLint param);
GLAPI void APIENTRY glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                                 GLenum format, GLenum type, GLvoid* pixels);
GLAPI void APIENTRY glGetIntegerv(GLenum pname, GLint* params);
GLAPI GLenum APIENTRY glGetError(void);
GLAPI const GLubyte* APIENTRY glGetString(GLenum name);

#ifdef __cplusplus
}
#endif

#endif /* SOFTGL_GL_H */
