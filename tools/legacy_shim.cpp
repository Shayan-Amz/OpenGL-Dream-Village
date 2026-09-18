// Compiles the untouched upstream program (legacy/main.cpp) into the
// head-less test binary, wrapped in a namespace so that its global functions
// do not collide with the port.  The system headers it names are included
// first so that they land in the global namespace as they should.  On
// non-Windows hosts <windows.h>/<mmsystem.h> resolve to empty stand-ins in
// tools/softgl/include/compat (the legacy code never uses them).  Nothing in
// legacy/main.cpp itself is changed.
//
// Note: the legacy program starts every frame with
//     void delay(){ for(int i=0;i<100000000;i++); }
// The loop has no observable effect, so any optimising compiler removes it
// (this file is built with -O2 / /O2); it is *not* patched out here.
#include <GL/glut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define main legacyMain
namespace legacy {
#include "../legacy/main.cpp"
}
#undef main

// ---------------------------------------------------------------------------
// Read-only access to the legacy program's animation counters (file-scope
// statics, hence only reachable from this translation unit).  The head-less
// harness uses it to render the port with *exactly* the state the legacy
// program just drew, which isolates drawing differences from animation ones.
#include "scene.h"

namespace legacy {

dv::AnimationState snapshotState()
{
    dv::AnimationState s;
    s.train = tmove;
    s.bigBoat = bmove;
    s.smallBoats = smallboat;
    const float clouds[5] = {cloud1, cloud2, cloud3, cloud4, cloud5};
    for (int i = 0; i < 5; ++i) s.cloud[i] = clouds[i];
    s.birdLeftward = birdmove1;
    s.birdRightward = birdmove2;
    const float oranges[6] = {orange1, orange2, orange3, orange4, orange5, orange6};
    for (int i = 0; i < 6; ++i) s.orange[i] = oranges[i];
    s.planeLeftward = pl1;
    s.planeRightward = pl2;
    s.diagBird1X = xx;
    s.diagBird1Y = yy;
    s.diagBird2X = txx;
    s.diagBird2Y = tyy;
    const float rains[12] = {r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12};
    for (int i = 0; i < 12; ++i) s.rain[i] = rains[i];
    return s;
}

} // namespace legacy
