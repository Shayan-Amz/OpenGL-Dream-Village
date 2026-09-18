// Portable include of the legacy fixed-function OpenGL + GLUT headers.
//
//   Windows  : FreeGLUT from the nupengl.core NuGet package (GL/glut.h)
//   Linux    : freeglut3-dev                                (GL/glut.h)
//   macOS    : Apple's deprecated but still shipped GLUT.framework
//   headless : tools/softgl/include is put first on the include path and
//              provides its own GL/gl.h + GL/glut.h (see tools/softgl)
#ifndef DREAM_VILLAGE_GL_INCLUDE_H
#define DREAM_VILLAGE_GL_INCLUDE_H

#if defined(__APPLE__) && !defined(DV_SOFTGL)
#  define GL_SILENCE_DEPRECATION 1
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

#endif // DREAM_VILLAGE_GL_INCLUDE_H
