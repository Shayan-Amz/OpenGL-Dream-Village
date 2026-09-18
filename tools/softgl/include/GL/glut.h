/*
 * softgl stand-in for <GL/glut.h>.  Declares the GLUT entry points the scene
 * uses; softgl implements them as no-ops so that the program links without a
 * window system.  The head-less harness (tools/render_headless.cpp) drives the
 * frame directly.  Enumerant values match freeglut's.
 */
#ifndef SOFTGL_GLUT_H
#define SOFTGL_GLUT_H

#include <GL/gl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GLUT_RGB    0x0000
#define GLUT_RGBA   0x0000
#define GLUT_SINGLE 0x0000
#define GLUT_DOUBLE 0x0002
#define GLUT_DEPTH  0x0010

#define GLUT_LEFT_BUTTON   0
#define GLUT_MIDDLE_BUTTON 1
#define GLUT_RIGHT_BUTTON  2
#define GLUT_DOWN 0
#define GLUT_UP   1

void glutInit(int* argc, char** argv);
void glutInitDisplayMode(unsigned int mode);
void glutInitWindowSize(int width, int height);
void glutInitWindowPosition(int x, int y);
int  glutCreateWindow(const char* title);
void glutDisplayFunc(void (*func)(void));
void glutIdleFunc(void (*func)(void));
void glutKeyboardFunc(void (*func)(unsigned char key, int x, int y));
void glutMouseFunc(void (*func)(int button, int state, int x, int y));
void glutReshapeFunc(void (*func)(int width, int height));
void glutTimerFunc(unsigned int msecs, void (*func)(int value), int value);
void glutMainLoop(void);
void glutSwapBuffers(void);
void glutPostRedisplay(void);

#ifdef __cplusplus
}
#endif

#endif /* SOFTGL_GLUT_H */
