# softgl — a tiny software rasteriser for testing

`softgl.c` implements, in under 600 lines of C99, the part of the OpenGL 1.1
fixed-function pipeline that Dream Village uses:

| Stage | What is implemented |
|---|---|
| Vertex specification | `glBegin`/`glEnd` with points, lines, line strip/loop, triangles, strip, fan, quads, quad strip, polygon (convex); `glVertex2{d,f,i}`, `glVertex3{d,f}`, `glColor3{ub,f,d}`, `glColor4f`, `glRect{f,d}` |
| Transformation | Modelview and projection stacks (depth 32), `glLoadIdentity`, `glPush/PopMatrix`, `glTranslate{d,f}`, `glScale{d,f}`, `glOrtho`, `glViewport`; vertices go through the combined projection × modelview matrix like a real driver |
| Rasterisation | Half-space triangle rasteriser with pixel-centre sampling and a top-left fill rule (shared edges are drawn exactly once), Gouraud colour interpolation, diamond-exit-style DDA lines (half-open), wide lines as quads |
| Framebuffer | RGB8, bottom row first; `glClear`, `glClearColor`, `glReadPixels(GL_RGB, GL_UNSIGNED_BYTE)` with `GL_PACK_ALIGNMENT` |
| Error model | `GL_INVALID_OPERATION` for calls inside `glBegin/glEnd` (and nested/unbalanced begin/end), `GL_STACK_{OVER,UNDER}FLOW`, `GL_INVALID_ENUM/VALUE` where the spec demands; `glGetError` semantics |
| GLUT | No-op stand-ins for the handful of `glut*` functions the scene links against, so the same object files work in the application and in the harness |

Not implemented (not needed by the scene): depth buffer, blending, textures,
lighting, anti-aliasing, flat shading, display lists.

## Why

There is no GPU, window system or vendor OpenGL on a CI runner, and the
scene's behaviour is *visual*.  softgl makes a frame a deterministic,
byte-comparable artefact, which allows two things:

1. **Regression testing of a refactor.** `render_headless --compare` draws a
   frame with the restructured `src/scene.cpp` and the same frame with the
   untouched `legacy/main.cpp` and counts differing pixels.  The restructuring
   is required to be pixel-identical (`--max-diff-ppm 0`).
2. **Measuring GL misuse.** Because the error model follows the spec, the
   harness can report that the legacy program raises 5 (day/night) or 605
   (rainy scenes) `GL_INVALID_OPERATION` errors per frame, and that the port
   raises none.

softgl is *not* a conformance-grade implementation; it agrees with real
implementations on the well-defined cases (interior pixels, shared edges)
and makes a fixed choice where the spec allows implementation variance
(e.g. line end-point rasterisation).

## Self-test

`selftest.c` checks the rasteriser against exact pixel counts: rectangle
coverage and edge exclusivity, no double coverage across a shared diagonal,
quad/polygon/fan equivalence, Gouraud endpoints, matrix stack behaviour and
underflow error, line lengths, `glLineWidth`-inside-`glBegin` error, and a
`glReadPixels` round trip.  It runs as the `softgl_selftest` CTest.
