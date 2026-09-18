// Dream Village — interactive front-end (GLUT).
//
//   keys   d / r / n / a   day, rain, night, rain at night
//          space           pause / resume the animation
//          p               save the current frame as PNG in the working directory
//          q / Esc         quit
//   mouse  left button     rain at night        right button   day
//
//   dream_village --screenshot DIR [--ticks N]
//          renders the four scenes off-screen (after N animation ticks,
//          default 40), writes DIR/{day,rain,night,rain_night}.png and exits.
//
// Frame pacing uses glutTimerFunc at a fixed tick rate; the upstream program
// paced itself with an empty 100-million-iteration loop, which optimising
// compilers delete entirely.  Double buffering replaces single-buffered
// rendering with glFlush(), which tears visibly on most drivers.
#include "gl_include.h"
#include "png_writer.h"
#include "scene.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

constexpr int      kWindowWidth   = 1300;   // the scene was authored for a 2:1 window
constexpr int      kWindowHeight  = 650;
constexpr unsigned kTickMillis    = 33;     // ~30 animation ticks per second
constexpr int      kDefaultTicks  = 40;     // animation phase used by --screenshot

dv::AnimationState g_state;
dv::Scene          g_scene  = dv::Scene::Day;
bool               g_paused = false;

// Letter-boxed viewport that keeps the 2:1 design aspect ratio.
int g_vpX = 0, g_vpY = 0, g_vpW = kWindowWidth, g_vpH = kWindowHeight;

// --screenshot mode
bool        g_screenshotMode = false;
std::string g_screenshotDir;
int         g_screenshotTicks = kDefaultTicks;

dv::Image captureViewport()
{
    std::vector<unsigned char> pixels(static_cast<size_t>(g_vpW) * g_vpH * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(g_vpX, g_vpY, g_vpW, g_vpH, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    return dv::imageFromGlPixels(g_vpW, g_vpH, pixels.data());
}

// Renders `scene` into the back buffer and writes it to `path` (no swap, so
// the visible frame is unaffected).
bool saveFrame(dv::Scene scene, const std::string& path)
{
    dv::render(scene, g_state);
    glFinish();
    const bool ok = dv::writePng(captureViewport(), path);
    std::printf("%s %s (%dx%d)\n", ok ? "wrote" : "could not write", path.c_str(), g_vpW, g_vpH);
    return ok;
}

int screenshotAll()
{
    dv::AnimationState fresh;
    g_state = fresh;
    for (int t = 0; t < g_screenshotTicks; ++t) dv::advance(g_state);

    bool ok = true;
    for (const dv::Scene sc : {dv::Scene::Day, dv::Scene::Rain, dv::Scene::Night, dv::Scene::RainNight})
        ok = saveFrame(sc, g_screenshotDir + "/" + dv::sceneName(sc) + ".png") && ok;
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

void display()
{
    if (g_screenshotMode) std::exit(screenshotAll());   // first frame: render, save, leave
    dv::render(g_scene, g_state);
    glutSwapBuffers();
}

void tick(int)
{
    if (!g_paused) dv::advance(g_state);
    glutPostRedisplay();
    glutTimerFunc(kTickMillis, tick, 0);
}

void reshape(int width, int height)
{
    if (width < 1) width = 1;
    if (height < 1) height = 1;
    // Largest 2:1 rectangle that fits, centred; the rest stays black.
    if (width >= 2 * height) { g_vpH = height;  g_vpW = 2 * height; }
    else                     { g_vpW = width;   g_vpH = width / 2;  }
    g_vpX = (width - g_vpW) / 2;
    g_vpY = (height - g_vpH) / 2;
    glViewport(g_vpX, g_vpY, g_vpW, g_vpH);
}

void keyboard(unsigned char key, int, int)
{
    switch (key) {
    case 'd': g_scene = dv::Scene::Day;       break;
    case 'r': g_scene = dv::Scene::Rain;      break;
    case 'n': g_scene = dv::Scene::Night;     break;
    case 'a': g_scene = dv::Scene::RainNight; break;
    case ' ': g_paused = !g_paused;           break;
    case 'p': saveFrame(g_scene, std::string("dream_village_") + dv::sceneName(g_scene) + ".png"); break;
    case 'q':
    case 27:  std::exit(EXIT_SUCCESS);
    default:  return;
    }
    glutPostRedisplay();
}

void mouse(int button, int state, int, int)
{
    if (state != GLUT_DOWN) return;
    if (button == GLUT_LEFT_BUTTON)       g_scene = dv::Scene::RainNight;
    else if (button == GLUT_RIGHT_BUTTON) g_scene = dv::Scene::Day;
    else return;
    glutPostRedisplay();
}

void usage(const char* argv0)
{
    std::printf("usage: %s [--screenshot DIR [--ticks N]]\n"
                "  keys: d/r/n/a scene, space pause, p screenshot, q quit\n", argv0);
}

} // namespace

int main(int argc, char** argv)
{
    glutInit(&argc, argv);                         // lets GLUT consume its own options first
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--screenshot" && i + 1 < argc) { g_screenshotMode = true; g_screenshotDir = argv[++i]; }
        else if (arg == "--ticks" && i + 1 < argc) { g_screenshotTicks = std::atoi(argv[++i]); }
        else if (arg == "--help" || arg == "-h")   { usage(argv[0]); return EXIT_SUCCESS; }
        else { usage(argv[0]); return 2; }
    }

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(kWindowWidth, kWindowHeight);
    glutCreateWindow("Dream Village");
    dv::setupProjection();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    if (!g_screenshotMode) {
        glutKeyboardFunc(keyboard);
        glutMouseFunc(mouse);
        glutTimerFunc(kTickMillis, tick, 0);
        std::printf("Dream Village — d/r/n/a: scene, space: pause, p: screenshot, q: quit\n");
    }
    glutMainLoop();
    return EXIT_SUCCESS;
}
