// Dream Village — scene model.
//
// The drawing routines in scene.cpp are the original hand-authored primitives
// from Krishno Dey's "Dream-Village" (see NOTICE), reorganised so that they
// are pure functions of an explicit AnimationState instead of mutating global
// counters while drawing.  Everything here is plain C++: no window system, no
// timers, no GLUT.  That is what makes the scene renderable both by the GLUT
// front-end (src/main.cpp) and by the head-less test harness
// (tools/render_headless.cpp) on top of the softgl software rasteriser.
#ifndef DREAM_VILLAGE_SCENE_H
#define DREAM_VILLAGE_SCENE_H

namespace dv {

// World space: an orthographic 100 x 100 unit square, y pointing up.
constexpr double kWorldWidth  = 100.0;
constexpr double kWorldHeight = 100.0;

// Every value that changes between frames.  The initial values and the
// per-tick increments in advance() are the ones the upstream program used.
struct AnimationState {
    float train         = 0.f;                  // vertical offset of the train (rolls downwards)
    float bigBoat       = 0.f;                  // vertical offset of the large sailing boat
    float smallBoats    = 0.f;                  // bobbing offset of the three small boats
    float cloud[5]      = {-20.f, -60.f, -80.f, -110.f, -130.f};   // horizontal offsets
    float birdLeftward  = 5.f;                  // bird flying right-to-left across the sky
    float birdRightward = -5.f;                 // bird flying left-to-right
    float orange[6]     = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f};          // falling fruit offsets
    float planeLeftward = 5.f;                  // night scene only
    float planeRightward = -7.f;
    float diagBird1X = 0.f, diagBird1Y = 0.f;   // bird crossing the village diagonally
    float diagBird2X = 0.f, diagBird2Y = 0.f;
    // Vertical offsets of the twelve rain layers, ten units apart.  Upstream
    // started them at +0 … +110 (above the screen), so rain took many frames
    // to fill the view; the port starts with the layers spread over the view.
    float rain[12]      = {0.f, -10.f, -20.f, -30.f, -40.f, -50.f,
                           -60.f, -70.f, -80.f, -90.f, -100.f, -110.f};
};

enum class Scene { Day, Rain, Night, RainNight };

const char* sceneName(Scene s);            // "day", "rain", "night", "rain_night"

// One simulation tick.  Upstream advanced the counters as a side effect of
// drawing, once per frame; the increments here are identical, so one tick
// corresponds to one upstream frame.
void advance(AnimationState& s);

// Draw one complete frame of the given scene with the current GL state
// (projection already set up by the caller, see setupProjection()).
void render(Scene scene, const AnimationState& s);

// Sets up the fixed 100 x 100 orthographic projection and the clear colour.
void setupProjection();

} // namespace dv

#endif // DREAM_VILLAGE_SCENE_H
