/* softgl extension API — only used by the head-less harness, never by the application. */
#ifndef SOFTGL_H
#define SOFTGL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct softgl_stats {
    unsigned long begin_end_pairs;   /* glBegin/glEnd blocks since the last reset */
    unsigned long vertices;          /* vertices submitted                          */
    unsigned long triangles;         /* triangles rasterised (quads/polygons split) */
    unsigned long lines;             /* line segments rasterised                    */
    unsigned long fragments;         /* pixels written (including overdraw)         */
    unsigned long errors;            /* GL errors raised (first one kept by glGetError) */
    unsigned long post_redisplay;    /* glutPostRedisplay calls                     */
} softgl_stats;

/* (Re)allocate the framebuffer.  Also resets the viewport to cover it. */
void softgl_create_framebuffer(int width, int height);
/* Pointer to the RGB8 framebuffer, bottom row first (OpenGL convention). */
const unsigned char* softgl_framebuffer(int* width, int* height);
void softgl_get_stats(softgl_stats* out);
void softgl_reset_stats(void);

#ifdef __cplusplus
}
#endif

#endif /* SOFTGL_H */
