# Dream Village — an animated 2D scene in immediate-mode OpenGL

[![CI](https://github.com/Shayan-Amz/OpenGL-Dream-Village/actions/workflows/ci.yml/badge.svg)](https://github.com/Shayan-Amz/OpenGL-Dream-Village/actions/workflows/ci.yml)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)](src/scene.cpp)
[![OpenGL 1.1 · FreeGLUT](https://img.shields.io/badge/OpenGL-1.1%20fixed--function%20%C2%B7%20FreeGLUT-5586A4?logo=opengl)](src/main.cpp)
[![Platforms](https://img.shields.io/badge/platforms-Windows%20%C2%B7%20Linux%20%C2%B7%20macOS-lightgrey)](.github/workflows/ci.yml)
[![License: MIT (own code)](https://img.shields.io/badge/license-MIT%20(own%20code)-yellow)](NOTICE)

An animated 2-D village — houses, an orchard, a railway, a river with boats, birds, planes, clouds
and rain — drawn with the OpenGL 1.1 fixed-function pipeline in four scenes: day, rain, night, and
rain at night. Everything moves: the train and the big boat run along the diagonal, the small boats
bob, the clouds drift, birds cross the sky, and in the two rainy scenes the rain falls.

<p align="center">
  <img src="docs/figures/scenes_gpu.png" alt="The four scenes: day, rain, night, rain at night" width="900">
  <br><sub>The four scenes as drawn by the real OpenGL build — Visual Studio 2022, Release | x64.</sub>
</p>

<p align="center">
  <img src="docs/figures/scenes_softgl.png" alt="The same four scenes rendered by the repository's own software rasteriser" width="820">
  <br><sub>The same four scenes rendered <em>head-less</em> with no GPU at all, by the software rasteriser in <a href="tools/softgl">tools/softgl</a> — the reference frames the regression tests compare against.</sub>
</p>

> **Provenance.** The scene geometry was authored by **Krishno Dey** and published as
> [krishnodey/Dream-Village](https://github.com/krishnodey/Dream-Village) (2020, Code::Blocks /
> MinGW, no licence). This repository is my port of it to Visual Studio 2022 and to a
> cross-platform CMake build; the file as received is kept verbatim in [`legacy/`](legacy/), and
> [`NOTICE`](NOTICE) states exactly what is whose. The MIT licence covers only the code written
> here.

## How the program is organised

The drawing code is split from the simulation, which is what makes the frames reproducible and
therefore testable:

* [`src/scene.h`](src/scene.h) / [`src/scene.cpp`](src/scene.cpp) — `dv::AnimationState`, the state
  of every moving object (train, boats, clouds, birds, planes, oranges, twelve rain layers),
  `dv::advance()` which steps it by one tick, and `dv::render(Scene, const AnimationState&)` which
  draws a frame and never modifies the state.
* [`src/main.cpp`](src/main.cpp) — the GLUT front end: a 1300 × 650 double-buffered window, a 33 ms
  timer that calls `advance()` and redraws, keyboard and mouse input, letter-boxing on resize, and
  `--screenshot DIR` to render all four scenes off-screen.
* [`src/png_writer.cpp`](src/png_writer.cpp) — `glReadPixels` → PNG, so the `p` key saves the current
  frame.

Everything is built from `GL_QUADS`, `GL_TRIANGLES`, convex `GL_POLYGON`s, `glRectf` and a
50-segment `GL_TRIANGLE_FAN` ellipse (`circle(rx, ry, x, y)`), with per-vertex colours for the simple
shading on the roofs and hulls. A day frame is ≈ 7 000 triangles and ≈ 1.25 M fragments at
1300 × 650 — cheap enough that the immediate-mode API is perfectly adequate here, even though it has
been deprecated since OpenGL 3.0.

## The scene

**World.** An orthographic 100 × 100 unit square (`glOrtho(0,100,0,100,-1,1)`), y up, shown in a
2:1 window — the design is stretched 2:1 horizontally, which is part of its look. There is no depth
buffer, so painter's order is the only visibility mechanism: sky (or night sky), road and railway,
village ground with houses and trees, river with boats, a white border, and finally the rain overlay.

**Animation.** The train and the big boat move along the diagonal tracks and the river; the small
boats bob; five cloud layers drift at different speeds; two birds fly horizontally, two more fly
diagonally, two planes cross the sky at night; six oranges drop from the tree; twelve rain layers
fall. Each of these is one field of `AnimationState`, stepped and wrapped by `advance()`.

**Scenes.** `Scene::{Day, Rain, Night, RainNight}` select the sky, the river palette and whether the
rain is drawn; everything else is shared.

## Building and running

### Windows — Visual Studio 2022

Open `DreamVillage.sln` and let NuGet restore `nupengl.core` (FreeGLUT headers, import libraries and
`freeglut.dll`), then build **Release | x64** and run. Or from a *Developer Command Prompt*:

```bat
nuget restore DreamVillage.sln
msbuild DreamVillage.sln -p:Configuration=Release -p:Platform=x64
x64\Release\DreamVillage.exe
```

### Linux / macOS — CMake

```bash
# Debian/Ubuntu: sudo apt install build-essential cmake freeglut3-dev
# macOS: Xcode command-line tools (GLUT.framework is included, deprecated but functional)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/dream_village
```

### Controls

| Input | Action |
|---|---|
| `d` `r` `n` `a` | day · rain · night · rain at night |
| left / right mouse button | rain at night · day |
| `space` | pause / resume |
| `p` | save the current frame as `dream_village_<scene>.png` |
| `q` / `Esc` | quit |
| `--screenshot DIR [--ticks N]` | render all four scenes off-screen after `N` ticks and exit |

## Testing without a GPU

Rendering is the program's only observable behaviour and CI runners have no GPU, display or vendor
OpenGL — so instead of mocking GL calls, [`tools/softgl`](tools/softgl) implements the subset of the
fixed-function pipeline this scene uses, in software: matrix stacks, orthographic projection,
viewport transform, a half-space triangle rasteriser with pixel-centre sampling and a top-left fill
rule, Gouraud interpolation, lines, `glReadPixels`, and the specification's error model. The scene
sources compile unchanged against softgl's `GL/gl.h`.

Two tests run under `ctest` on Ubuntu, macOS and Windows (MSVC):

* **`headless_render_matches_legacy`** renders each scene off-screen and requires the frame to be
  **pixel-identical** to the reference drawing code compiled into the same binary (zero differing
  pixels). It also reports the GL error count, the triangle and fragment counts and the fraction of
  the frame covered:

  ```
  scene       triangles fragments coverage errors
  day              7017   1248754    92.6%      0
  rain             7017   1255347    92.7%      0
  night            6989   1219474    92.5%      0
  rain_night       6989   1225364    92.6%      0
  ```

* **`softgl_selftest`** checks the rasteriser itself against exact pixel counts (edge exclusivity, no
  double coverage across shared edges, Gouraud endpoints, matrix-stack errors, line lengths, error
  semantics).

Exact pixel counts only mean something if the arithmetic is reproducible, so the build disables
multiply-add contraction (`-ffp-contract=off` for GCC and Clang; MSVC's default `/fp:precise` does
not contract). With contraction on, Clang fuses `a*b+c` into an FMA, edge functions of pixels lying
exactly on a triangle edge stop being exactly zero, and the fill rule sends those pixels to both
triangles or to neither — the counts then change from compiler to compiler.

```bash
ctest --test-dir build --output-on-failure
./build/render_headless --out out --compare        # writes out/{day,rain,night,rain_night}.png
```

## Repository layout

```
src/
├── scene.h / scene.cpp     AnimationState, advance(), render() and the drawing routines
├── main.cpp                GLUT front-end: window, timer, input, screenshots
├── png_writer.h/.cpp       glReadPixels → PNG (stb_image_write)
└── gl_include.h            platform GL/GLUT include
tools/
├── softgl/                 software rasteriser (softgl.c, headers, self-test, README)
├── render_headless.cpp     head-less renderer + frame comparison (CTest)
└── legacy_shim.cpp         compiles the reference drawing code into the test binary
legacy/                     the program the scene came from (Krishno Dey), verbatim
third_party/                stb_image_write.h (public domain / MIT)
docs/figures/               rendered scenes used in this README
CMakeLists.txt              Linux/macOS/Windows build + tests
DreamVillage.sln/.vcxproj   Visual Studio 2022 solution (nupengl.core via NuGet)
.github/workflows/ci.yml    Ubuntu GCC · macOS Clang · Windows MSVC (VS solution + CTest)
```

## Limitations and possible extensions

* The scene uses the **immediate-mode API** (`glBegin`/`glEnd`), which is deprecated since OpenGL 3.0
  and unavailable in core profiles. A modern version would bake the static geometry once into a
  vertex buffer and draw the animated objects with per-object model matrices — the `AnimationState`
  split already isolates exactly the data such a renderer would need.
* Time is measured in **ticks**, not seconds; a very slow machine would slow the animation down
  rather than drop frames. A `std::chrono`-based accumulator in `tick()` would decouple the two.
* softgl covers only what this scene needs (no depth test, blending or textures) and is intended as a
  test oracle, not as a general renderer.
* The design is authored for a 2:1 aspect ratio and simply letter-boxes otherwise; there is no
  high-DPI handling beyond what GLUT provides.

## References

* Mark Segal, Kurt Akeley, *The OpenGL Graphics System: A Specification, Version 1.1* — §2.6
  Begin/End paradigm, §2.10 coordinate transformations, §3.4–3.5 line and polygon rasterisation,
  §2.5 GL errors.
* Juan Pineda, "A Parallel Algorithm for Polygon Rasterization", *SIGGRAPH 1988* — the half-space
  (edge-function) rasterisation used by softgl.
* Mark Kilgard, *The OpenGL Utility Toolkit (GLUT) Programming Interface, API Version 3*; FreeGLUT
  documentation for the open-source implementation used here.
* Sean Barrett, `stb_image_write.h` — single-header PNG writer.

## License

Code written for this repository (everything outside `legacy/` and the upstream-authored geometry in
`src/scene.cpp`) — MIT, see [`LICENSE`](LICENSE). Upstream geometry — © Krishno Dey, used with
attribution, see [`NOTICE`](NOTICE).
