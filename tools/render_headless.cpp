// Head-less renderer and regression check for Dream Village.
//
// Links the scene against the softgl software rasteriser (tools/softgl)
// instead of a real OpenGL driver, renders the four scenes at a fixed
// animation phase and writes them as PNG.  With --compare it also renders the
// *legacy* program (legacy/main.cpp, compiled into this binary inside a
// namespace) at the same phase and counts differing pixels, so the
// restructuring of the drawing code can be checked pixel by pixel.
//
//   render_headless [--out DIR] [--ticks N] [--width W --height H]
//                   [--compare [--max-diff-ppm X]]
//
// Exit status 0 = PASS, 1 = a scene raised GL errors / rendered (almost)
// nothing / differed from the legacy frame by more than --max-diff-ppm.
#include "gl_include.h"
#include "png_writer.h"
#include "scene.h"
#include "softgl.h"

#include <cstdio>
#include <cstdlib>
#include <string>

#ifdef DV_WITH_LEGACY
namespace legacy {
void init();
void dayMode();
void rainyMode();
void nightMode();
void rainyModeNight();
dv::AnimationState snapshotState();   // defined in legacy_shim.cpp
} // namespace legacy
#endif

namespace {

dv::Image grab()
{
    int w = 0, h = 0;
    const unsigned char* fb = softgl_framebuffer(&w, &h);
    return dv::imageFromGlPixels(w, h, fb);
}

size_t countDifferent(const dv::Image& a, const dv::Image& b, dv::Image* mask)
{
    size_t n = 0;
    for (size_t i = 0; i + 2 < a.rgb.size(); i += 3) {
        const bool differ = a.rgb[i] != b.rgb[i] || a.rgb[i + 1] != b.rgb[i + 1] || a.rgb[i + 2] != b.rgb[i + 2];
        if (differ) ++n;
        if (mask) mask->rgb[i] = mask->rgb[i + 1] = mask->rgb[i + 2] = differ ? 255 : 0;
    }
    return n;
}

double coveragePercent(const dv::Image& a)
{
    size_t n = 0;
    for (size_t i = 0; i + 2 < a.rgb.size(); i += 3)
        if (a.rgb[i] | a.rgb[i + 1] | a.rgb[i + 2]) ++n;
    return 100.0 * static_cast<double>(n) / (static_cast<double>(a.width) * a.height);
}

#ifdef DV_WITH_LEGACY
// The legacy program never calls glMatrixMode: its init() applies glOrtho to
// the *modelview* matrix of a fresh context.  Reproduce that state exactly.
void setupLegacyMatrices()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    legacy::init();
}

void drawLegacy(dv::Scene sc)
{
    setupLegacyMatrices();
    switch (sc) {
    case dv::Scene::Day:       legacy::dayMode();        break;
    case dv::Scene::Rain:      legacy::rainyMode();      break;
    case dv::Scene::Night:     legacy::nightMode();      break;
    case dv::Scene::RainNight: legacy::rainyModeNight(); break;
    }
}
#endif

} // namespace

int main(int argc, char** argv)
{
    std::string outDir = ".";
    int ticks = 40, width = 1300, height = 650;
    bool compare = false;
    double maxDiffPpm = 0.0;

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        const bool hasValue = i + 1 < argc;
        if (a == "--out" && hasValue) outDir = argv[++i];
        else if (a == "--ticks" && hasValue) ticks = std::atoi(argv[++i]);
        else if (a == "--width" && hasValue) width = std::atoi(argv[++i]);
        else if (a == "--height" && hasValue) height = std::atoi(argv[++i]);
        else if (a == "--compare") compare = true;
        else if (a == "--max-diff-ppm" && hasValue) maxDiffPpm = std::atof(argv[++i]);
        else { std::fprintf(stderr, "usage: %s [--out DIR] [--ticks N] [--width W --height H] [--compare [--max-diff-ppm X]]\n", argv[0]); return 2; }
    }
#ifndef DV_WITH_LEGACY
    if (compare) { std::fprintf(stderr, "this binary was built without the legacy program (DV_WITH_LEGACY)\n"); return 2; }
#endif

    softgl_create_framebuffer(width, height);
    dv::setupProjection();
    dv::AnimationState state;
    for (int t = 0; t < ticks; ++t) dv::advance(state);

#ifdef DV_WITH_LEGACY
    if (compare) {
        // The legacy program advances its counters as a side effect of drawing
        // (one step per frame), so `ticks` day-mode frames put it in the same
        // phase as `ticks` calls to dv::advance().  Every legacy frame drawn
        // afterwards advances the counters once more; the port is kept in
        // lock-step by calling advance() before each comparison frame.
        for (int t = 0; t < ticks; ++t) drawLegacy(dv::Scene::Day);
    }
#endif

    const dv::Scene scenes[] = {dv::Scene::Day, dv::Scene::Rain, dv::Scene::Night, dv::Scene::RainNight};
    bool ok = true;
    std::printf("%-11s %9s %9s %8s %6s %4s", "scene", "triangles", "fragments", "coverage", "errors", "png");
    if (compare) std::printf(" %10s %10s %12s %10s", "legacy_err", "draw_diff", "draw_ppm", "anim_diff");
    std::printf("\n");

    for (const dv::Scene sc : scenes) {
        if (compare) dv::advance(state);

        dv::setupProjection();
        softgl_reset_stats();
        dv::render(sc, state);
        softgl_stats st{};
        softgl_get_stats(&st);
        const dv::Image mine = grab();
        const double coverage = coveragePercent(mine);
        const bool saved = dv::writePng(mine, outDir + "/" + dv::sceneName(sc) + ".png");
        std::printf("%-11s %9lu %9lu %7.1f%% %6lu %4s", dv::sceneName(sc), st.triangles, st.fragments,
                    coverage, st.errors, saved ? "ok" : "FAIL");
        if (st.errors || !saved || coverage < 90.0) ok = false;

#ifdef DV_WITH_LEGACY
        if (compare) {
            // 1. Legacy frame (this also advances the legacy counters).
            const dv::AnimationState before = legacy::snapshotState();
            softgl_reset_stats();
            drawLegacy(sc);
            softgl_stats lst{};
            softgl_get_stats(&lst);
            const dv::Image ref = grab();

            // 2. Port frame drawn from the counters the legacy frame actually
            //    used -> isolates differences in the *drawing* code.  Every
            //    legacy counter is stepped once, just before its object is
            //    drawn, so the post-frame value is the drawn one — except the
            //    rain: the legacy update routine decrements all twelve layer
            //    offsets and is called once per layer, so layer i was drawn
            //    after i of those twelve decrements (a 1-unit skew per layer).
            dv::AnimationState used = legacy::snapshotState();
            for (int i = 0; i < 12; ++i) {
                float r = before.rain[i];
                for (int j = 0; j < i; ++j) { r -= 1.f; if (r < -100.f) r = 0.f; }
                used.rain[i] = r;
            }
            dv::setupProjection();
            dv::render(sc, used);
            const dv::Image sameState = grab();
            dv::Image mask = sameState;
            const size_t diffDraw = countDifferent(sameState, ref, &mask);

            // 3. Port frame from its own advance() -> adds animation drift.
            const size_t diffAnim = countDifferent(mine, ref, nullptr);

            const double total = static_cast<double>(mine.width) * mine.height;
            std::printf(" %10lu %10zu %12.1f %10zu", lst.errors, diffDraw, 1e6 * diffDraw / total, diffAnim);
            dv::writePng(ref, outDir + "/legacy_" + dv::sceneName(sc) + ".png");
            dv::writePng(mask, outDir + "/diff_" + dv::sceneName(sc) + ".png");
            dv::writePng(sameState, outDir + "/port_samestate_" + dv::sceneName(sc) + ".png");
            if (1e6 * diffDraw / total > maxDiffPpm) ok = false;
        }
#endif
        std::printf("\n");
    }
    std::printf("%s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
