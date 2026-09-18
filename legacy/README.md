# `legacy/` — the program as received

`main.cpp` is the upstream program exactly as it was in this repository
before the restructuring (Krishno Dey's Dream-Village with the Visual Studio
port changes, see [NOTICE](../NOTICE)).  It is kept for two reasons:

1. **Provenance** — the diff between this file and `src/scene.cpp` is the
   honest record of what was changed.
2. **Regression oracle** — `tools/legacy_shim.cpp` compiles it, unmodified,
   into the head-less test binary, which renders it with the softgl
   rasteriser next to the restructured scene and requires the two frames to be
   pixel-identical (`ctest` → `headless_render_matches_legacy`).

It is **not** built into the interactive application.

## Defects that the restructuring fixed

| # | In `legacy/main.cpp` | Effect | Fix in `src/` |
|---|---|---|---|
| 1 | `void delay(){ for(int i=0;i<100000000;i++); }` at the start of every frame | Frame pacing by busy-waiting: optimising compilers delete the loop (animation runs at an uncontrolled rate), debug builds burn a CPU core | `glutTimerFunc` tick at 30 Hz, decoupled from drawing |
| 2 | `GLUT_SINGLE` + `glFlush()` | Single-buffered rendering: visible tearing/flicker while the frame is redrawn | `GLUT_DOUBLE` + `glutSwapBuffers()` |
| 3 | `glLineWidth(20)` / `glLineWidth(50)` **inside** `glBegin/glEnd` (3 sites; the rain one runs 600× per frame) | `GL_INVALID_OPERATION` — the call is ignored, all "wide" lines are 1 px | removed (the 1 px look is preserved) |
| 4 | `glBegin(GL_QUADS)` without `glEnd()` in `homes()` (footpath #3) | Nested `glBegin` → `GL_INVALID_OPERATION`; the second quad silently merges into the first primitive | `glEnd()` inserted |
| 5 | Animation counters mutated from *inside* drawing routines, plus 29 `glutPostRedisplay()` calls per frame | Drawing is not repeatable, animation speed depends on which scene is shown, `glutPostRedisplay` spam | `dv::AnimationState` + `dv::advance()`; drawing is a pure function of the state |
| 6 | `if(cloud5 > 200){ cloud4 = -130; }` | Cloud 5 never wraps; cloud 4 jumps whenever cloud 5 passes the edge | wrap the right variable |
| 7 | `circle()` iterates 100 steps over `2 * PI` with `PI = 2.0f * 3.1416` (= 4π) | Every ellipse is drawn twice (≈120 ellipses per frame → 6 100 redundant triangles: 13 142 vs 7 017 triangles per day frame) | one turn, 50 segments, same vertices |
| 8 | Rain offsets `r1…r12` decremented by the per-layer draw routine — 12 decrements per frame with a 1-unit skew between layers, and only while a rainy scene is displayed | Rain speed and spacing are artefacts of the call order | fixed fall speed per tick, layers wrap modulo their spacing |
| 9 | `#include <windows.h>`, `<mmsystem.h>` (unused) | Windows-only for no reason | removed; builds on Linux/macOS with freeglut/GLUT |
| 10 | `glOrtho` applied to the modelview matrix, no reshape handler | Resizing the window distorts the scene | projection matrix, letter-boxed `reshape()` |
| 12 | `glColor3ub(0, 0, .10)` for the house eyes and `house1(1.5, 15)` | Both literals are converted to an integer type, so the eyes are (0, 0, 0) rather than the intended dark blue and the house sits at x = 1 | Behaviour preserved for pixel parity and written as an explicit cast in `src/scene.cpp`, with a comment |
| 11 | Four near-identical scene functions (`dayMode`, `rainyMode`, `nightMode`, `rainyModeNight`) | Copy-paste drift (day draws sky before road, the others after) | one `render(Scene, state)` |

Items 1, 3, 4 and 7 are also *measured* by the test harness: the legacy frame
raises 5 GL errors in the day/night scenes and 605 in the rainy ones
(`legacy_err` column), the port raises none.
