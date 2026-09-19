// Dream Village — drawing routines.
//
// Provenance: the primitives below (houses, trees, trains, boats, birds,
// planes, clouds, rain, sky, river, road) are the hand-authored geometry of
// Krishno Dey's "Dream-Village" (github.com/krishnodey/Dream-Village, 2020),
// see NOTICE.  This file keeps that geometry and colour palette intact and
// changes only the *structure*:
//
//   * animation counters live in dv::AnimationState and are advanced by
//     dv::advance() — drawing no longer mutates global state or calls
//     glutPostRedisplay() (upstream did so 29 times per frame);
//   * the four scene functions collapse into dv::render(Scene, state);
//   * GL usage errors are fixed: an unterminated glBegin() in homes(),
//     glLineWidth() calls inside glBegin/glEnd (GL_INVALID_OPERATION,
//     hundreds of times per rainy frame), a duplicated circle loop and a
//     wrap-around typo (cloud 5 reset cloud 4);
//   * the deliberately slow busy-wait delay() and the Windows-only includes
//     are gone; frame pacing is the front-end's job (glutTimerFunc).
//
// Coordinates are in world units of the 100 x 100 orthographic view.
#include "scene.h"
#include "gl_include.h"

#include <cmath>

namespace dv {

namespace {

constexpr int    kCircleSegments = 50;
constexpr double kRainFallPerTick = 2.0;   // world units per tick, see advance()
constexpr double kRainPeriod = 120.0;      // 12 layers x 10 units, see advance()

// The animation state the primitives read while drawing a frame.  render()
// sets it; nothing else touches it.
const AnimationState* g_state = nullptr;
#define anim (*g_state)

// Filled ellipse with radii rx, ry centred on (x, y), as a 50-segment fan.
//
// Upstream computed the rim angles as `2 * PI * i / 100` with `PI = 2.0f * 3.1416`
// — its "PI" was already 2*pi, so the 100-step loop ran twice around the
// circle and drew the same 50-gon twice.  One turn with the identical
// single-precision arithmetic puts every vertex on exactly the same pixels.
void circle(GLfloat rx, GLfloat ry, GLfloat x, GLfloat y)
{
    const GLfloat twoPi = static_cast<GLfloat>(2.0 * 3.1416);   // sic, see above
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x,y);
    for(int i=0;i<=kCircleSegments;i++)
    {
        // Float arithmetic, exactly as in the upstream program (the divisor is
        // exact in both int and float); no implicit int -> float conversions.
        const float angle = (2.f * twoPi * static_cast<float>(i)) /
                            static_cast<float>(2 * kCircleSegments);
        glVertex2f(x+(cos(angle)*rx),y+(sin(angle)* ry));
    }
    glEnd();
}

void orangeFall()
{
    glColor3ub(255,165,0);
    glPushMatrix();
        glTranslated(0, anim.orange[0], 0);
        circle(.5,1,33,37);
            glPopMatrix();
    glPushMatrix();
        glTranslated(0, anim.orange[1], 0);
        circle(.5,1,35,39);
    glPopMatrix();

    glPushMatrix();
        glTranslated(0, anim.orange[2], 0);
        circle(.5,1,38,41);

        glPopMatrix();
    glPushMatrix();
        glTranslated(0, anim.orange[3], 0);
        circle(.5,1,41,39);
    glPopMatrix();

    glPushMatrix();
        glTranslated(0, anim.orange[4], 0);
        circle(.5,1,43,41);
    glPopMatrix();

    glPushMatrix();
        glTranslated(0, anim.orange[5], 0);
        circle(.5,1,45,39);
    glPopMatrix();
}

void boat(float x1, float y1, float boatLength)
{
    float x2 = x1 + boatLength, y2 = y1, x3 = x2+(boatLength/2)-1,y3 = y2+boatLength/2, x4 = x2, y4 = y3-1, x5 = x1, y5 = y4, x6=x1-boatLength/2-1,y6=y3;

    glBegin(GL_POLYGON);
        glColor3d(1,0,0);
         glColor3ub(85, 28, 22);
        glVertex2d(x1,y1);
        glVertex2d(x2,y2);
        glVertex2d(x3,y3);
        glVertex2d(x4,y4);
        glVertex2d(x5,y5);
        glVertex2d(x6,y6);
    glEnd();

    glBegin(GL_POLYGON);
        glColor3d(1,1,1);

        glColor3ub(160, 155, 136);
        glVertex2d(x3,y3);
        glVertex2d(x4,y4);
        glVertex2d(x5,y5);
        glVertex2d(x6,y6);
        glVertex2d(x1,y6+.7);
        glVertex2d(x2,y3+.7);
    glEnd();
    glColor3d(.90,1,1);
    glRectf(static_cast<GLfloat>(x5-.2),y5+1,static_cast<GLfloat>(x5+.2),y5+12);

    glBegin(GL_TRIANGLES);
        glColor3d(0,.1,1);
        glVertex2d(x5+.5,y5+11);
        glVertex2d(x5+.5,y5+2.5);
        glVertex2d(x4-3,y5+2.3);
    glEnd();

    glBegin(GL_LINES);
        glColor3d(1,0,0);
        glVertex2d(x5+.5,y5+11);
        glVertex2d(x5+.2,y5+11);

        glVertex2d(x5+.5,y5+2.5);
        glVertex2d(x5+.2,y5+2.5);

        glVertex2d(x4-3,y5+2.3);
        glVertex2d(x4-2.8,y5+2);
    glEnd();
}

void train()
{
    glBegin(GL_QUADS);

    //back cabin
    glColor3ub(58, 90, 145);
    //top
    glVertex2d(61,55);
    glVertex2d(70,55);
    glVertex2d(69,59);
    glVertex2d(62,59);
    //front
    glVertex2d(61,55);
    glVertex2d(61,52);
    glVertex2d(70,52);
    glVertex2d(70,55);
    //front plate
    glColor3ub(225, 43, 145);
    glVertex2d(61.2,52);
    glVertex2d(61.2,48);
    glVertex2d(69.8,48);
    glVertex2d(69.8,52);
    //front plate small for aesthetics
    glColor3ub(22, 23, 145);
    glVertex2d(63,51);
    glVertex2d(63,49);
    glVertex2d(68.4,49);
    glVertex2d(68.4,51);

    //joint
    glColor3f(1,0,0);
    glVertex2d(65.1,48);
    glVertex2d(66.1,48);
    glVertex2d(66,46);
    glColor3f(1,static_cast<GLfloat>(.9),static_cast<GLfloat>(.30));
    glVertex2d(65,46);
    //joint shadow
    glColor3f(1,static_cast<GLfloat>(.9),0);
    glVertex2d(64.8,48);
    glVertex2d(65.1,48);
    glVertex2d(65,46);
    glVertex2d(64.7,46);

    //front cabin
    glColor3ub(58, 90, 145);
    //top
    glVertex2d(61,43);
    glVertex2d(70,43);
    glVertex2d(69,46);
    glVertex2d(62,46);
    //front
    glVertex2d(61,43);
    glVertex2d(61,39);
    glVertex2d(70,39);
    glVertex2d(70,43);

    //front plate
    glColor3ub(225, 43, 145);
    glVertex2d(61.2,40);
    glVertex2d(61.2,35);
    glVertex2d(69.8,35);
    glVertex2d(69.8,40);

    //front plate small for aesthetics
    glColor3ub(22, 23, 145);
    glVertex2d(63,39);
    glVertex2d(63,36);
    glVertex2d(68.4,36);
    glVertex2d(68.4,39);

    glEnd();
    //top plate front cabin
    glColor3f(1,0,0);
    circle(0.5,static_cast<GLfloat>(0.7),62,42);
    circle(0.5,static_cast<GLfloat>(0.7),69,42);
    //front plate
    glColor3f(1,1,1);
    circle(0.5,static_cast<GLfloat>(0.7),64,37.5);
    circle(0.5,static_cast<GLfloat>(0.7),65.5,37.5);
    circle(0.5,static_cast<GLfloat>(0.7),static_cast<GLfloat>(67.3),37.5);

    //rear cabin
    glColor3f(1,1,1);
    circle(static_cast<GLfloat>(0.3),0.5,63.5,static_cast<GLfloat>(50.2));
    circle(static_cast<GLfloat>(0.3),0.5,65,static_cast<GLfloat>(50.2));
    circle(static_cast<GLfloat>(0.3),0.5,66.5,static_cast<GLfloat>(50.2));
    circle(static_cast<GLfloat>(0.3),0.5,static_cast<GLfloat>(67.7),static_cast<GLfloat>(50.2));

}

void movingTrain()
{
    glPushMatrix();
        glTranslatef(0,anim.train+1,0);
        train();
        glPopMatrix();
}

void boat2(float x1=92,  float y1=40)
{

    float x2=x1+2,y2=y1+4,x3=x2,y3=y2+7,x4=x1,y4=y3+4,x5=x4-2,y5=y3,x6=x5,y6=y2;
   glBegin(GL_POLYGON);
        glColor3d(.91,.90,.91);
        glColor3ub(160, 155, 136);
        glColor3ub(45, 28, 22);
        glVertex2d(x1,y1);
        glVertex2d(x2,y2);
        glVertex2d(x3,y3);
        glVertex2d(x4,y4);
        glVertex2d(x5,y5);
        glVertex2d(x6,y6);
    glEnd();

    glBegin(GL_TRIANGLES);
        glColor3d(.1,0.2,0.2);
        glColor3ub(160, 155, 136);
         glVertex2d(x1,y1);
        glVertex2d(x2,y2);
        glVertex2d(x2-.7,y2);

        glVertex2d(x3,y3);
        glVertex2d(x4,y4);
        glVertex2d(x3-.7,y3);

        glVertex2d(x1,y1);
        glVertex2d(x6,y6);
        glVertex2d(x6+.7,y6);

        glVertex2d(x4,y4);
        glVertex2d(x5,y5);
        glVertex2d(x5+.7,y5);

        glVertex2d(x1,y1);
        glVertex2d(x1+1,y1+2);
        glVertex2d(x1-1,y1+2);

        glVertex2d(x4,y4);
        glVertex2d(x4+1,y4-2);
        glVertex2d(x4-1,y4-2);

    glEnd();

    glColor3ub(160, 155, 136);
    glRectf(x3,y3,static_cast<GLfloat>(x2-.7),y2);
    glRectf(x6,y6,static_cast<GLfloat>(x5+.7),y5);

    glRectf(static_cast<GLfloat>(x2-.7),y2-1,static_cast<GLfloat>(x6+.7),static_cast<GLfloat>(y2-1.5));
    glRectf(static_cast<GLfloat>(x2-.7),y2,static_cast<GLfloat>(x6+.7),static_cast<GLfloat>(y2+.5));
    glRectf(static_cast<GLfloat>(x2-.7),y2+2,static_cast<GLfloat>(x6+.7),static_cast<GLfloat>(y2+2.5));
    glRectf(static_cast<GLfloat>(x2-.7),y2+4,static_cast<GLfloat>(x6+.7),static_cast<GLfloat>(y2+4.5));
    glRectf(static_cast<GLfloat>(x2-.7),y2+6,static_cast<GLfloat>(x6+.7),static_cast<GLfloat>(y2+6.5));
    glRectf(static_cast<GLfloat>(x2-.7),static_cast<GLfloat>(y2+7.5),static_cast<GLfloat>(x6+.7),y2+8);

    glBegin(GL_QUADS);
        glVertex2d(x1-.2,y2+.2);
        glVertex2d(x1+.2,y2);
        glVertex2d(x2+1,y2+14);
        glVertex2d(x2+1,y2+14.4);

        glVertex2d(x1+.2,y2-.4);
        glVertex2d(x1+.6,y2-.2);
        glVertex2d(x1+1.4,y1-.6);
        glVertex2d(x1+1,y1-1);

        glVertex2d(x1+1.4,y1-.6);
        glVertex2d(x1+1,y1-1);
        glVertex2d(x1+.4,y1-1.6);
        glVertex2d(x1+2.1,y1-1.6);

    glEnd();

    glBegin(GL_LINES);
        glColor3f(1,0,0);
        glVertex2d(x1-.4,y2);
        glVertex2d(x1-.4,y3);

        glVertex2d(x1-.4,y3);
        glVertex2d(x4,y4);

        glVertex2d(x1-.4,y2);
        glVertex2d(x1,y1);

    glEnd();

    glBegin(GL_TRIANGLES);
         glColor3ub(160, 15, 16);
        glVertex2d(x4+.1, y4);
        glVertex2d(x2+.8, y2+14);
        glVertex2d(x1, y2+1);
    glEnd();

}

void movingBigBoat()
{
    glPushMatrix();
    glTranslatef(0, anim.bigBoat +1, 0);
    boat2();
    glPopMatrix();

}

void movingSmallBoat()
{
    glPushMatrix();
        glTranslated(0,anim.smallBoats,0);
        boat(76,40,7);
        boat(77,20,6);
        boat(79,3,5);
    glPopMatrix();

}

void tree1(float x1, float y1)
{
    float x2 = x1+4,y2=y1, x3=x1+2,y3=y1+5;
    glColor3ub(11, 70, 11);
    glBegin(GL_TRIANGLES);
        glVertex2d(x1, y1);
        glVertex2d(x2, y2);
        glVertex2d(x3, y3);
    glEnd();
    glBegin(GL_QUADS);
        glVertex2d(x1+1.5, y1);
        glVertex2d(x1, y1-4);
        glVertex2d(x2, y1-4);
        glVertex2d(x2-1.5, y1);

        glVertex2d(x1+1.5, y1-4);
        glVertex2d(x1, y1-8);
        glVertex2d(x2, y1-8);
        glVertex2d(x2-1.5, y1-4);
        glColor3ub(68, 43, 2);
        glVertex2d(x1+1.5, y1-8);
        glVertex2d(x1+1, y1-14);
        glVertex2d(x2-1, y1-14);
        glVertex2d(x2-1.5, y1-8);
    glEnd();
}

void tree2(float x1, float y1)
{
    float x2 = x1+5,y2=y1, x3=static_cast<GLfloat>(x1+2.5),y3=y1+6;
    glColor3ub(11, 50, 11);
    glBegin(GL_TRIANGLES);
        glVertex2d(x1, y1);
        glVertex2d(x2, y2);
        glVertex2d(x3, y3);
    glEnd();
    glBegin(GL_QUADS);
        glVertex2d(x1+2, y1);
        glVertex2d(x1, y1-5);
        glVertex2d(x2, y1-5);
        glVertex2d(x2-2, y1);

        glVertex2d(x1+2, y1-5);
        glVertex2d(x1, y1-10);
        glVertex2d(x2, y1-10);
        glVertex2d(x2-2, y1-5);
        glColor3ub(68, 43, 2);
        glVertex2d(x1+2, y1-10);
        glVertex2d(x1+1.5, y1-18);
        glVertex2d(x2-1.5, y1-18);
        glVertex2d(x2-2, y1-10);
    glEnd();
}

void tree3(float x1, float y1)
{
    float x2 = x1+2,y2=y1, x3=x1+1,y3=y1+3;
    glColor3ub(11, 70, 11);
    glBegin(GL_TRIANGLES);
        glVertex2d(x1, y1);
        glVertex2d(x2, y2);
        glVertex2d(x3, y3);
    glEnd();
    glBegin(GL_QUADS);
        glVertex2d(x1+.5, y1);
        glVertex2d(x1, y1-3);
        glVertex2d(x2, y1-3);
        glVertex2d(x2-.5, y1);

        glVertex2d(x1+.5, y1-3);
        glVertex2d(x1, y1-6);
        glVertex2d(x2, y1-6);
        glVertex2d(x2-.5, y1-3);

        glVertex2d(x1+.5, y1-6);
        glVertex2d(x1, y1-9);
        glVertex2d(x2, y1-9);
        glVertex2d(x2-.5, y1-6);

        glVertex2d(x1+.5, y1-9);
        glVertex2d(x1, y1-12);
        glVertex2d(x2, y1-12);
        glVertex2d(x2-.5, y1-9);

        glColor3ub(68, 43, 2);
        glColor3ub(68, 43, 2);
        glVertex2d(x1+.7, y1-12);
        glVertex2d(x1+.4, y1-20);
        glVertex2d(x2-.4, y1-20);
        glVertex2d(x2-.7, y1-12);
    glEnd();
}

void tree4(float x1, float y1)
{
    float x2 = x1+2,y2=y1, x3=x1+1,y3=y1+2;
    glColor3ub(11, 70, 11);
    glBegin(GL_TRIANGLES);
        glVertex2d(x1, y1);
        glVertex2d(x2, y2);
        glVertex2d(x3, y3);
    glEnd();
    glBegin(GL_QUADS);
        glVertex2d(x1+.5, y1);
        glVertex2d(x1, y1-2);
        glVertex2d(x2, y1-2);
        glVertex2d(x2-.5, y1);

        glVertex2d(x1+.5, y1-2);
        glVertex2d(x1, y1-4);
        glVertex2d(x2, y1-4);
        glVertex2d(x2-.5, y1-2);

        glVertex2d(x1+.5, y1-4);
        glVertex2d(x1, y1-6);
        glVertex2d(x2, y1-6);
        glVertex2d(x2-.5, y1-4);

        glVertex2d(x1+.5, y1-6);
        glVertex2d(x1, y1-8);
        glVertex2d(x2, y1-8);
        glVertex2d(x2-.5, y1-6);

        glVertex2d(x1+.5, y1-8);
        glVertex2d(x1, y1-10);
        glVertex2d(x2, y1-10);
        glVertex2d(x2-.5, y1-8);

        glColor3ub(68, 43, 2);
        glVertex2d(x1+.7, y1-10);
        glVertex2d(x1+.5, y1-17);
        glVertex2d(x2-.5, y1-17);
        glVertex2d(x2-.7, y1-10);
    glEnd();
}
void house1(int x, int y)
{
    glColor3f(0,0,0);
    glBegin(GL_TRIANGLES);
        glVertex2d(x, y);
        glVertex2d(x+2,y+4);
        glVertex2d(x+4,y-2);
    glEnd();

    glColor3f(1,static_cast<GLfloat>(0.7),static_cast<GLfloat>(.11));
    glBegin(GL_QUADS);
        //wall right
        glColor3f(static_cast<GLfloat>(.19),static_cast<GLfloat>(0.3),static_cast<GLfloat>(.30));
        glVertex2f(static_cast<GLfloat>(x+3.5), static_cast<GLfloat>(y-7));
        glVertex2f(static_cast<GLfloat>(x+7.3), static_cast<GLfloat>(y-4.5));
        glVertex2f(static_cast<GLfloat>(x+7.3), static_cast<GLfloat>(y-1));
        glVertex2f(static_cast<GLfloat>(x+3.5), static_cast<GLfloat>(y-1.5));
        //door
        glColor3f(static_cast<GLfloat>(.9),static_cast<GLfloat>(0.9),static_cast<GLfloat>(0.90));
        glVertex2f(static_cast<GLfloat>(x+5.5),static_cast<GLfloat>(y-5.5));
        glVertex2f(static_cast<GLfloat>(x+6.25),static_cast<GLfloat>(y-5));
        glVertex2f(static_cast<GLfloat>(x+6.25),static_cast<GLfloat>(y-2.5));
        glVertex2f(static_cast<GLfloat>(x+5.5),static_cast<GLfloat>(y-2.5));

        //right roof
        glColor3f(1,static_cast<GLfloat>(0.7),static_cast<GLfloat>(.11));
        glVertex2d(x+2,y+4);
        glVertex2d(x+6,y+5);
        glVertex2d(x+8,y-1);
        glVertex2d(x+4,y-2);

        //wall left
        glColor3f(static_cast<GLfloat>(.91),static_cast<GLfloat>(0.3),static_cast<GLfloat>(.31));
        glVertex2f(static_cast<GLfloat>(x +0.3), static_cast<GLfloat>(y-5));
        glVertex2f(static_cast<GLfloat>(x + 3.5), static_cast<GLfloat>(y-7));
        glVertex2f(static_cast<GLfloat>(x + 3.5), static_cast<GLfloat>(y-1.5));
        glVertex2f(static_cast<GLfloat>(x+0.5), static_cast<GLfloat>(y));

        //window left
        glColor3f(1,1,1);
        glVertex2f(static_cast<GLfloat>(x+1.5),static_cast<GLfloat>(y-3.5));
        glVertex2f(static_cast<GLfloat>(x+2.2),static_cast<GLfloat>(y-4));
        glVertex2f(static_cast<GLfloat>(x+2.2),static_cast<GLfloat>(y-1.7));
        glVertex2f(static_cast<GLfloat>(x+1.5),static_cast<GLfloat>(y-1.3));
    glEnd();
}

void house2(int x, int y)
{
    glColor3f(0,0,0);
    glBegin(GL_TRIANGLES);
        glVertex2d(x, y);
        glVertex2d(x+2,y+4);
        glVertex2d(x+4,y-2);
    glEnd();

    glColor3f(1,static_cast<GLfloat>(0.7),static_cast<GLfloat>(.11));
    glBegin(GL_QUADS);
        //wall right
        glColor3f(static_cast<GLfloat>(.3),static_cast<GLfloat>(0.1),static_cast<GLfloat>(.30));
        glVertex2f(static_cast<GLfloat>(x+3.5), static_cast<GLfloat>(y-7));
        glVertex2f(static_cast<GLfloat>(x+7.3), static_cast<GLfloat>(y-4.5));
        glVertex2f(static_cast<GLfloat>(x+7.3), static_cast<GLfloat>(y-1));
        glVertex2f(static_cast<GLfloat>(x+3.5), static_cast<GLfloat>(y-1.5));
        //door
        glColor3f(1,static_cast<GLfloat>(0.44),static_cast<GLfloat>(0.40));
        glVertex2f(static_cast<GLfloat>(x+5.5),static_cast<GLfloat>(y-5.5));
        glVertex2f(static_cast<GLfloat>(x+6.25),static_cast<GLfloat>(y-5));
        glVertex2f(static_cast<GLfloat>(x+6.25),static_cast<GLfloat>(y-2.5));
        glVertex2f(static_cast<GLfloat>(x+5.5),static_cast<GLfloat>(y-2.5));
        //right roof
        glColor3f(1,static_cast<GLfloat>(0.4),static_cast<GLfloat>(.31));
        glVertex2d(x+2,y+4);
        glVertex2d(x+6,y+5);
        glVertex2d(x+8,y-1);
        glVertex2d(x+4,y-2);
        //wall left
        glColor3f(static_cast<GLfloat>(.41),0.5,static_cast<GLfloat>(.31));
        glVertex2f(static_cast<GLfloat>(x +0.3), static_cast<GLfloat>(y-5));
        glVertex2f(static_cast<GLfloat>(x + 3.5), static_cast<GLfloat>(y-7));
        glVertex2f(static_cast<GLfloat>(x + 3.5), static_cast<GLfloat>(y-1.5));
        glVertex2f(static_cast<GLfloat>(x+0.5), static_cast<GLfloat>(y));

        //window left
        glColor3f(1,.0,static_cast<GLfloat>(.1));
        glVertex2f(static_cast<GLfloat>(x+1.5),static_cast<GLfloat>(y-3.5));
        glVertex2f(static_cast<GLfloat>(x+2.2),static_cast<GLfloat>(y-4));
        glVertex2f(static_cast<GLfloat>(x+2.2),static_cast<GLfloat>(y-1.7));
        glVertex2f(static_cast<GLfloat>(x+1.5),static_cast<GLfloat>(y-1.3));
    glEnd();
}

void bird1(int x, int y)
{

    //feater
    glColor3ub(206, 69, 19);
    glBegin(GL_QUADS);
        glVertex2f(static_cast<GLfloat>(x),static_cast<GLfloat>(y));
        glColor3f(static_cast<GLfloat>(.2), 1.0, static_cast<GLfloat>(.10));
        glVertex2f(static_cast<GLfloat>(x+5),static_cast<GLfloat>(y));
        glColor3f(1, static_cast<GLfloat>(.90), static_cast<GLfloat>(.160));
        glVertex2f(static_cast<GLfloat>(x+3.5),static_cast<GLfloat>(y-2.5));
        glColor3f(1.0, static_cast<GLfloat>(.20), static_cast<GLfloat>(.90));
        glVertex2f(static_cast<GLfloat>(x+1.5),static_cast<GLfloat>(y-2.5));

        //tail
    glEnd();
    //body
    glColor3ub(255, 9, 6);
    circle(2.5,2,static_cast<GLfloat>(x+2.55),static_cast<GLfloat>(y-4.55));

    glBegin(GL_TRIANGLES);
        //lips
        glColor3ub(55, 220, 60);
        glVertex2f(static_cast<GLfloat>(x-1.3),static_cast<GLfloat>(y-5));
        glVertex2f(static_cast<GLfloat>(x+.8),static_cast<GLfloat>(y-3.1));
        glColor3f(1, static_cast<GLfloat>(.20), static_cast<GLfloat>(.160));
        glVertex2f(static_cast<GLfloat>(x+.8),static_cast<GLfloat>(y-6));

        //tail
        glColor3ub(0, 220, 60);
        glVertex2f(static_cast<GLfloat>(x+4.8),static_cast<GLfloat>(y-5));
        glColor3f(1, static_cast<GLfloat>(.20), static_cast<GLfloat>(.160));
        glVertex2f(static_cast<GLfloat>(x+7),static_cast<GLfloat>(y-5));
        glColor3f(1, static_cast<GLfloat>(.60), static_cast<GLfloat>(.90));
        glVertex2f(static_cast<GLfloat>(x+7),static_cast<GLfloat>(y-1));
    glEnd();
    //eye
    // Upstream passes the literal 0.10 to a GLubyte parameter, so the value
    // is truncated to 0: the eyes render black.  Made explicit here to keep
    // the pixels identical to the legacy frame without a lossy-conversion
    // warning.
    glColor3ub(0, 0, static_cast<GLubyte>(.10));
    circle(static_cast<GLfloat>(.3),static_cast<GLfloat>(.41),static_cast<GLfloat>(x+.1),static_cast<GLfloat>(y-4.6));

    //legs
    glBegin(GL_QUADS);
        glColor3f(static_cast<GLfloat>(.01),80,static_cast<GLfloat>(.12));
        glVertex2f(static_cast<GLfloat>(x+1.5),static_cast<GLfloat>(y-6.5));
        glVertex2f(static_cast<GLfloat>(x+1.5),static_cast<GLfloat>(y-8));
        glVertex2f(static_cast<GLfloat>(x+2),static_cast<GLfloat>(y-8));
        glVertex2f(static_cast<GLfloat>(x+2),static_cast<GLfloat>(y-6.5));

        glColor3f(static_cast<GLfloat>(.01),90,static_cast<GLfloat>(.12));
        glVertex2f(static_cast<GLfloat>(x+2.5),static_cast<GLfloat>(y-6.5));
        glVertex2f(static_cast<GLfloat>(x+2.5),static_cast<GLfloat>(y-8));
        glVertex2f(static_cast<GLfloat>(x+3),static_cast<GLfloat>(y-8));
        glVertex2f(static_cast<GLfloat>(x+3),static_cast<GLfloat>(y-6.5));
    glEnd();
    glColor3f(1,static_cast<GLfloat>(.60),static_cast<GLfloat>(.32));
    glBegin(GL_TRIANGLES);
    //1 red front
        glVertex2f(static_cast<GLfloat>(x+1.5),static_cast<GLfloat>(y-8));
        glVertex2f(static_cast<GLfloat>(x+2.5),static_cast<GLfloat>(y-8));
        glVertex2f(static_cast<GLfloat>(x+.5),static_cast<GLfloat>(y-8.5));

        glVertex2f(static_cast<GLfloat>(x+1.5),static_cast<GLfloat>(y-8));
        glVertex2f(static_cast<GLfloat>(x+2),static_cast<GLfloat>(y-8));
        glVertex2f(static_cast<GLfloat>(x+1.5),static_cast<GLfloat>(y-8.9));

        glVertex2f(static_cast<GLfloat>(x+1.5),static_cast<GLfloat>(y-8));
        glVertex2f(static_cast<GLfloat>(x+2),static_cast<GLfloat>(y-8));
        glVertex2f(static_cast<GLfloat>(x+2.5),static_cast<GLfloat>(y-8.9));

    //1 red rear leg
        glVertex2d(x+2.5,y-8);
        glVertex2d(x+3,y-8);
        glVertex2d(x+2,y-8.8);

        glVertex2d(x+2.5,y-8);
        glVertex2d(x+3,y-8);
        glVertex2d(x+2.9,y-9.4);

        glVertex2d(x+2.5,y-8);
        glVertex2d(x+3,y-8);
        glVertex2d(x+4,y-9);
    glEnd();
}
void bird2(int x, int y)
{
    //feater
    glColor3ub(206, 69, 19);
    glBegin(GL_QUADS);
        glVertex2f(static_cast<GLfloat>(x),static_cast<GLfloat>(y));
        glColor3f(static_cast<GLfloat>(.9), 1.0, static_cast<GLfloat>(.10));
        glVertex2f(static_cast<GLfloat>(x+5),static_cast<GLfloat>(y));
        glColor3f(1, 0, static_cast<GLfloat>(.160));
        glVertex2f(static_cast<GLfloat>(x+3.5),static_cast<GLfloat>(y-2.5));
        glColor3f(1.0, 0.0, 1.0);
        glVertex2f(static_cast<GLfloat>(x+1.5),static_cast<GLfloat>(y-2.5));

    glEnd();
    //body
    glColor3f(1, 1, 0);
    circle(2.5,2,static_cast<GLfloat>(x+2.55),static_cast<GLfloat>(y-4.55));

    glBegin(GL_TRIANGLES);
        //lips
        glColor3ub(55, 220, 60);
        glVertex2f(static_cast<GLfloat>(x+4.5),static_cast<GLfloat>(y-3.3));
        glVertex2f(static_cast<GLfloat>(x+4.5),static_cast<GLfloat>(y-5.8));
        glColor3f(1, static_cast<GLfloat>(.290), static_cast<GLfloat>(.160));
        glVertex2f(static_cast<GLfloat>(x+6),static_cast<GLfloat>(y-4.4));

        //tail
        glColor3ub(0, 220, 60);
        glVertex2f(static_cast<GLfloat>(x+.7),static_cast<GLfloat>(y-5.3));
        glColor3f(1, static_cast<GLfloat>(.20), static_cast<GLfloat>(.160));
        glVertex2f(static_cast<GLfloat>(x-2),static_cast<GLfloat>(y-2));
        glColor3f(1, static_cast<GLfloat>(.9), static_cast<GLfloat>(.90));
        glVertex2f(static_cast<GLfloat>(x-2),static_cast<GLfloat>(y-5.5));
    glEnd();
    //eye
    // Same truncation as in house1(); see the note there.
    glColor3ub(0, 0, static_cast<GLubyte>(.10));
    circle(static_cast<GLfloat>(.3),static_cast<GLfloat>(.41),static_cast<GLfloat>(x+5),static_cast<GLfloat>(y-4.4));

    //legs
    glBegin(GL_QUADS);
        glColor3f(static_cast<GLfloat>(.01),80,static_cast<GLfloat>(.12));
        glVertex2f(static_cast<GLfloat>(x+1.7),static_cast<GLfloat>(y-6.5));
        glVertex2f(static_cast<GLfloat>(x+1.7),static_cast<GLfloat>(y-8));
        glVertex2f(static_cast<GLfloat>(x+2.2),static_cast<GLfloat>(y-8));
        glVertex2f(static_cast<GLfloat>(x+2.2),static_cast<GLfloat>(y-6.5));

        glColor3f(static_cast<GLfloat>(.01),90,static_cast<GLfloat>(.12));
        glVertex2f(static_cast<GLfloat>(x+2.7),static_cast<GLfloat>(y-6.5));
        glVertex2f(static_cast<GLfloat>(x+2.7),static_cast<GLfloat>(y-8));
        glVertex2f(static_cast<GLfloat>(x+3.2),static_cast<GLfloat>(y-8));
        glVertex2f(static_cast<GLfloat>(x+3.2),static_cast<GLfloat>(y-6.5));
    glEnd();

    glBegin(GL_TRIANGLES);
        glColor3f(1,static_cast<GLfloat>(.3),0);
        //1 red front
        glVertex2f(static_cast<GLfloat>(x+1.5),static_cast<GLfloat>(y-8));
        glVertex2f(static_cast<GLfloat>(x+2.5),static_cast<GLfloat>(y-8));
        glVertex2f(static_cast<GLfloat>(x+.5),static_cast<GLfloat>(y-8.5));

        glVertex2f(static_cast<GLfloat>(x+1.5),static_cast<GLfloat>(y-8));
        glVertex2f(static_cast<GLfloat>(x+2),static_cast<GLfloat>(y-8));
        glVertex2f(static_cast<GLfloat>(x+1.5),static_cast<GLfloat>(y-8.9));

        glVertex2f(static_cast<GLfloat>(x+1.5),static_cast<GLfloat>(y-8));
        glVertex2f(static_cast<GLfloat>(x+2),static_cast<GLfloat>(y-8));
        glVertex2f(static_cast<GLfloat>(x+2.5),static_cast<GLfloat>(y-8.9));

    //1 red rear leg
        glVertex2d(x+2.7,y-8);
        glVertex2d(x+3.2,y-8);
        glVertex2d(x+2,y-8.8);

        glVertex2d(x+2.7,y-8);
        glVertex2d(x+3.2,y-8);
        glVertex2d(x+2.9,y-9.4);

        glVertex2d(x+2.7,y-8);
        glVertex2d(x+3.2,y-8);
        glVertex2d(x+4,y-9);
    glEnd();
}

void MovingBird()
{
    //1st bird left to right
    glPushMatrix();
    glTranslatef(anim.birdLeftward+1,0,0);
    bird1(90,95);
    glPopMatrix();

    //2nd bird right to left
    glPushMatrix();
    glTranslatef(anim.birdRightward+1,0,0);
    bird2(7,75);
    glPopMatrix();

}

void Plane1()
{
    glBegin(GL_POLYGON);
    glColor3ub(33,69,200);
        glVertex2f(88,90);
        glVertex2f(92,90);
        glVertex2f(93,94);
        glVertex2f(88,92);
        glVertex2f(86,90);
    glEnd();
    glColor3f(1,1,1);
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),88.5,91);
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),89.5,91);
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),90.5,91);
    glColor3f(1,0,0);
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),92.5,static_cast<GLfloat>(93.4));
}

void Plane2()
{
    //left one
    glBegin(GL_POLYGON);
    glColor3ub(33,69,200);

        glVertex2f(3,84);
        glVertex2f(4,80);
        glVertex2f(8,80);
        glVertex2f(10,80);
        glVertex2f(8,82);

    glEnd();

    glColor3f(1,1,1);
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),5,static_cast<GLfloat>(81.2));
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),static_cast<GLfloat>(6.4),static_cast<GLfloat>(81.2));
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),static_cast<GLfloat>(7.8),static_cast<GLfloat>(81.2));
    glColor3f(1,0,0);
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),3.5,static_cast<GLfloat>(83.4));

}

void planeMove()
{
    glPushMatrix();
        glTranslated(anim.planeLeftward,0,0);
        Plane1();
    glPopMatrix();

    glPushMatrix();
        glTranslated(anim.planeRightward,0,0);
        Plane2();
    glPopMatrix();
}

void diagonalBirdMove()
{
    glPushMatrix();
        glTranslated(anim.diagBird1X+1,anim.diagBird1Y+3,0);
        bird1(32,48);
    glPopMatrix();
    glPushMatrix();
        glTranslated(anim.diagBird2X+1,anim.diagBird2Y,0);
        bird2(42,49);
    glPopMatrix();

}

// One layer of rain: fifty short vertical strokes across the top of the scene.
// (Upstream set glLineWidth(20) *inside* glBegin/glEnd, which is a GL error and
// has no effect, so the strokes were always one pixel wide.  Kept that way.)
void Rain()
{
    glColor3f(1,1,1);
    int y = 97;
    for(int i = 0;i<100;i= i+2){
        glBegin(GL_LINES);
        glVertex2d(i,y);
        glVertex2d(i,y-2);
    glEnd();

    }
}

// Twelve copies of the rain layer, each shifted by its own vertical offset.
void rainFall()
{
    for (float offset : anim.rain) {
        glPushMatrix();
        glTranslated(0, offset, 0);
        Rain();
        glPopMatrix();
    }
}

void river()
{
    glColor3ub(1, 42, 104);

    circle(3,5,77.5,50);
    circle(3,5,78,42);
    circle(3,5,78.5,35);
    circle(3,5,79.5,27);
    circle(3,5,80,20);
    circle(3,5,80.5,14);
    circle(3,5,80.5,7);

    glBegin(GL_QUADS);
        glVertex2d(75, 55);
        glVertex2d(80, 2);
        glVertex2d(98.5, 2);
        glVertex2d(98.5, 55);

    glEnd();

    movingSmallBoat();

    movingBigBoat();
}

void nightRiver()
{
    glColor3ub(36, 51, 94);

    circle(3,5,77.5,50);
    circle(3,5,78,42);
    circle(3,5,78.5,35);
    circle(3,5,79.5,27);
    circle(3,5,80,20);
    circle(3,5,80.5,14);
    circle(3,5,80.5,7);

    glBegin(GL_QUADS);
        glVertex2d(75, 55);
        glVertex2d(80, 2);
        glVertex2d(98.5, 2);
        glVertex2d(98.5, 55);

    glEnd();

    movingSmallBoat();
    movingBigBoat();
}

void rightTriangle(float x, float y, float distance,float height)
{
    glBegin(GL_TRIANGLES);
        glVertex2d(x,y);
        glVertex2d(x+.5,y-distance);
        glVertex2d(x-height,(y+y-distance)/2);
    glEnd();
}

void leftTriangle(float x, float y, float distance,float height)
{
    glBegin(GL_TRIANGLES);
        glVertex2d(x,y);
        glVertex2d(x,y-distance);
        glVertex2d(x+height,(y+y-distance)/2);
    glEnd();
}
void road()
{
    glColor3f(1, 1, 1);
    glColor3ub(190,190,190);
    glBegin(GL_QUADS);
        glVertex2d(75, 2);
        glVertex2d(70, 60);
        glVertex2d(60, 60);
        glVertex2d(60, 2);

    glEnd();
    //railpath right side footpath
    glBegin(GL_POLYGON);
     glColor3ub(22, 58, 21);
        glVertex2d(70, 60);
        glVertex2d(75, 55);
        glVertex2d(80, 2);
        glVertex2d(75, 2);
    glEnd();

    //triangles
    glColor3ub(22, 58, 21);
    rightTriangle(70,60,2,3);
    rightTriangle(static_cast<GLfloat>(70.3),58,2,3);
    rightTriangle(static_cast<GLfloat>(70.6),56,2,3);
    rightTriangle(static_cast<GLfloat>(70.9),54,2,3);
    rightTriangle(71,52,2,2.5);
    rightTriangle(static_cast<GLfloat>(71.2),50,2,2.5);
    rightTriangle(static_cast<GLfloat>(71.4),48,2,static_cast<GLfloat>(2.3));
    float x =static_cast<GLfloat>(71.6), y=46.0;
    int i;
    for(i=1; i<23; i++){
        rightTriangle(x, y, 2.0,2.0);
        x = static_cast<float>(x + 0.2);       // upstream: x += 0.2 in double arithmetic
        y -= 2;
    }

    //leftt side
    glColor3ub(22, 58, 21);
    leftTriangle(60,60,2,3);
    leftTriangle(60,58,2,3);
    leftTriangle(60,56,2,3);
    leftTriangle(60,54,2,static_cast<GLfloat>(2.4));
    leftTriangle(60,52,2,static_cast<GLfloat>(2.2));
    leftTriangle(60,50,2,2);
    leftTriangle(60,48,1,1.5);
    leftTriangle(60,47,1,1.5);


    float a=41.5;
    for(int j=0;j<15;j++){
        leftTriangle(60,a,1,1);
           a = a -1;
    }
     a=21;
    for(int j=0;j<14;j++){
        leftTriangle(60,a,1,1);
           a = a -1;

    }
    leftTriangle(60,3,1,1);

    //railroad
    glColor3f(0,0,0);
    glRectf(64,59,66,58);
    glRectf(63.5,57,66.5,56);
    glRectf(63,55,67,54);
    glRectf(62.5,53,67.5,52);
    glRectf(62,51,68,50);
    glRectf(61.5,49,68.5,48);
    glRectf(static_cast<GLfloat>(61.3),47,69,46);
    glRectf(static_cast<GLfloat>(61.2),45,static_cast<GLfloat>(69.3),44);
    glRectf(static_cast<GLfloat>(61.1),43,69.5,42);
    glRectf(61,41,static_cast<GLfloat>(69.7),40);
    glRectf(61,39,static_cast<GLfloat>(69.8),38);
    glRectf(static_cast<GLfloat>(61.1),37,static_cast<GLfloat>(69.9),36);
    glRectf(static_cast<GLfloat>(61.05),35,70,34);
    glRectf(61,33,static_cast<GLfloat>(70.2),32);
    glRectf(61,31,static_cast<GLfloat>(70.4),30);
    glRectf(61,29,static_cast<GLfloat>(70.6),28);
    glRectf(61,27,static_cast<GLfloat>(70.8),26);
    glRectf(61,25,71,24);
    glRectf(61,23,static_cast<GLfloat>(71.2),22);
    glRectf(61,21,static_cast<GLfloat>(71.4),20);
    glRectf(61,19,static_cast<GLfloat>(71.6),18);
    glRectf(61,17,static_cast<GLfloat>(71.8),16);
    glRectf(61,15,72,14);
    glRectf(61,13,static_cast<GLfloat>(72.2),12);
    glRectf(61,11,72.5,10);
    glRectf(61,9,static_cast<GLfloat>(72.7),8);
    glRectf(61,7,static_cast<GLfloat>(72.9),6);
    glRectf(61,5,73,4);
    glRectf(61,3,static_cast<GLfloat>(73.3),2);

 //rail pati
    glBegin(GL_POLYGON);
    glColor3ub(104, 92, 62);
    glVertex2d(62.5,2);
    glVertex2d(62.5,45);
    glVertex2d(63,45);
    glVertex2d(63,2);
    glEnd();

    glBegin(GL_POLYGON);
    glColor3ub(104, 92, 62);
    glVertex2d(62.5,45);
    glVertex2d(64.5,60);
    glVertex2d(64.8,60);
    glVertex2d(63,45);
    glEnd();

    glBegin(GL_POLYGON);
    glColor3ub(104, 92, 62);
    glVertex2d(69,2);
    glVertex2d(68.4,2);
    glVertex2d(68,45);
    glVertex2d(68.5,45);
    glEnd();

    glBegin(GL_POLYGON);
    glColor3ub(104, 92, 62);
    glVertex2d(65.4,60);
    glVertex2d(65,60);
    glVertex2d(68,45);
    glVertex2d(68.5,45);
    glEnd();

    movingTrain();

}

void movingCloud()
{
    glColor3f(1,1,1);
    glPushMatrix();
        glTranslatef(anim.cloud[0],1,0);
        circle(3,5,5,88);
        circle(3,5,8,91);
        circle(3,5,12,92);
        circle(3,5,12,87);
        circle(3,5,7,85);
    glPopMatrix();

    glPushMatrix();
        glTranslatef(anim.cloud[1],-2,0);
        circle(3,5,30,88);
        circle(3,5,33,91);
        circle(3,5,37,92);
        circle(3,5,37,87);
        circle(3,5,32,85);

    glPopMatrix();

    glPushMatrix();
        glTranslatef(anim.cloud[2],-3,0);
        circle(3,5,5,88);
        circle(3,5,8,91);
        circle(3,5,12,92);
        circle(3,5,12,87);
        circle(3,5,7,85);

    glPopMatrix();
    glPushMatrix();
        glTranslatef(anim.cloud[3],1,0);
        circle(3,5,30,88);
        circle(3,5,33,91);
        circle(3,5,37,92);
        circle(3,5,37,87);
        circle(3,5,32,85);

    glPopMatrix();

    glPushMatrix();
        glTranslatef(anim.cloud[4],1,0);
        circle(3,5,5,88);
        circle(3,5,8,91);
        circle(3,5,12,92);
        circle(3,5,12,87);
        circle(3,5,7,85);
    glPopMatrix();

}
void sky()
{
    glColor3ub(14, 138, 239);
    glBegin(GL_POLYGON);
        glVertex2d(1.5,60);
        glVertex2d(1.5, 98);
        glVertex2d(98.5, 98);
        glVertex2d(98.5, 60);
    glEnd();
    glBegin(GL_QUADS);
        glVertex2d(70,60);
        glVertex2d(75,55);
        glVertex2d(98.5,55);
        glVertex2d(98.5,60);
    glEnd();

    glColor3ub(255, 220, 0);
    circle(4,9,75,85);
    //cloud left to right
    glColor3f(1, 1, 1);

    //static cloud
    circle(3,4,78,71);
    circle(3,4,75,67);
    circle(3,4,78,62);
    circle(3,4,80,65);

    //horizontal flying cloud
    movingCloud();
    //horizontal bird flying
    MovingBird();

}

void star(float x1, float y1,float distance, float height)
{
    float x2 = x1 + distance, y2 = y1, x3 = x2, y3 = y2 + distance, x4 = x1, y4 = y3;

    glBegin(GL_QUADS);
        glVertex2f(x1, y1);
        glVertex2f(x2, y2);

        glVertex2f(x3, y3);
        glVertex2f(x4, y4);
    glEnd();
   glBegin(GL_TRIANGLES);
    //bottom
        glVertex2f(x1, y1);
        glVertex2f(x2, y2);
        glVertex2f(x1+(distance/2) , y2-height);
    //right
        glVertex2f(x2, y2);
        glVertex2f(x3, y3);
        glVertex2f(x2+height,y2+(distance/2));
    //top
        glVertex2f(x3,y3);
        glVertex2f(x4,y4);
        glVertex2f(x1+(distance/2), y4+height);
    //left
        glVertex2f(x4,y4);
        glVertex2f(x1,y1);
        glVertex2f(x1-height,y1+(distance/2));
    glEnd();

}
void nightsky()
{
    glColor3ub(6, 0, 45);
    glBegin(GL_POLYGON);
        glVertex2d(1.5,60);
        glVertex2d(1.5, 98);
        glVertex2d(98.5, 98);
        glVertex2d(98.5, 60);
    glEnd();
    glBegin(GL_QUADS);
        glVertex2d(70,60);
        glVertex2d(75,55);
        glVertex2d(98.5,55);
        glVertex2d(98.5,60);
    glEnd();

    glColor3ub(251, 252, 234);
    //moon
    circle(4,9,75,85);

    //stars
    star(5,97,static_cast<GLfloat>(0.4),static_cast<GLfloat>(0.8));
    star(10,66,static_cast<GLfloat>(0.4),static_cast<GLfloat>(0.8));
    star(25,90,static_cast<GLfloat>(0.4),static_cast<GLfloat>(0.8));
    star(30,71,static_cast<GLfloat>(0.4),static_cast<GLfloat>(0.8));
    star(45,95,static_cast<GLfloat>(0.4),static_cast<GLfloat>(0.8));
    star(60,75,static_cast<GLfloat>(0.4),static_cast<GLfloat>(0.8));
    star(76,65,static_cast<GLfloat>(0.4),static_cast<GLfloat>(0.8));
    star(86,85,static_cast<GLfloat>(0.4),static_cast<GLfloat>(0.8));
    //circle star
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),2,88);
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),3,68);
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),12,78);
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),24,69);
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),19,97);
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),35,98);
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),32,82);
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),50,73);
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),55,91);
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),68,96);
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),62,66);
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),80,61);
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),81,81);
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),95,98);
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),98,75);
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),96,58);

    star(78,53,static_cast<GLfloat>(0.4),static_cast<GLfloat>(0.8));
    star(85,50,static_cast<GLfloat>(0.4),static_cast<GLfloat>(0.8));
    star(95,48,static_cast<GLfloat>(0.4),static_cast<GLfloat>(0.8));
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),98,52);
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),83,53);
    circle(static_cast<GLfloat>(.2),static_cast<GLfloat>(.3),88,54);

    planeMove();
    MovingBird();
}

void homes()
{
    glColor3ub(22, 49, 22);
    glBegin(GL_POLYGON);
        glVertex2d(1.5,60);
        glVertex2d(60,60);
        glVertex2d(60,2);
        glVertex2d(1.5,2);
    glEnd();

    glColor3ub(22, 49, 22);
    circle(2,3,8,60);
    circle(2,3,11,60);
    circle(1,2,13,60);

    circle(1,2,28,60);
    circle(2,3,30.5,60);
    circle(2,3,33.5,60);

    circle(1,2,51,60);
    circle(2,3,53.5,60);
    circle(2,3,56.5,60);

    //footpath in the village #1
    glColor3ub(104, 92, 62);
    glBegin(GL_QUADS);
    glVertex2d(1.5,46);
    glVertex2d(1.5,42);
    glColor3ub(40, 40, 39);
    glVertex2d(60.5,42);
    glVertex2d(60.5,46);

    glVertex2d(60.5,42);
    glVertex2d(60.5,41.6);
    glVertex2d(60,41.6);
    glVertex2d(60,42);

    glColor3ub(33, 32, 30);
    glVertex2d(1.5,42);
    glVertex2d(1.5,41);
    glVertex2d(60,41);
    glVertex2d(60,42);
    glEnd();

    //footpath in the village #2
    glColor3ub(104, 92, 62);
    glBegin(GL_QUADS);
    glVertex2d(1.5,26);
    glVertex2d(1.5,22);
    glColor3ub(40, 40, 39);
    glVertex2d(60.5,22);
    glVertex2d(60.5,26);

    glVertex2d(60.5,22);
    glVertex2d(60.5,21.6);
    glVertex2d(60,21.6);
    glVertex2d(60,22);

    glColor3ub(33, 32, 30);
    glVertex2d(1.5,22);
    glVertex2d(1.5,21);
    glVertex2d(60,21);
    glVertex2d(60,22);
    glEnd();

    //footpath in the village #3
    glColor3ub(104, 92, 62);
    glBegin(GL_QUADS);
    glVertex2d(1.5,7);
    glVertex2d(1.5,4);
    glColor3ub(40, 40, 39);
    glVertex2d(60.5,4);
    glVertex2d(60.5,7);

    glVertex2d(60.5,4);
    glVertex2d(60.5,3.6);
    glVertex2d(60,3.6);
    glVertex2d(60,4);
    glEnd();                       // was missing: the original nested a second glBegin here
    glColor3ub(33, 32, 30);
    glBegin(GL_QUADS);
    glVertex2d(1.5,4);
    glVertex2d(1.5,3);
    glVertex2d(60,3);
    glVertex2d(60,4);
    glEnd();

    house1(7,55);
    house2(27,55);
    house1(50,55);
    house1(4,35);
    house2(23,35);
    house1(47,35);
    // Upstream calls house1(1.5, 15); the parameter is int, so the fractional
    // part is dropped and the house sits at x = 1.  Spelled out to keep the
    // geometry identical to the legacy frame without a conversion warning.
    house1(static_cast<int>(1.5), 15);
    house2(21,15);
    house1(44,15);

    //2
    glColor3ub(68, 83, 2);
    circle(1,1,6,55);
    tree3(5,75);

    //trees 1
    glColor3ub(68, 83, 2);
    circle(1.5,1.5,4,49);
    tree1(2,63);

    //3
    glColor3ub(68, 83, 2);
    circle(1,1,16,55);
    tree3(15,75);

    //4
    glColor3ub(68, 83, 2);
    circle(1.5,1.5,18.5,49);
    tree2(16,67);
    //5
    glColor3ub(68, 83, 2);
    circle(1.5,1.5,24,49);
    tree1(22,63);
    //6
    glColor3ub(68, 83, 2);
    circle(1,1,27,57);
    tree3(26,77);

    //7
    glColor3ub(68, 83, 2);
    circle(1.5,1.5,37.5,49);
    tree2(35,67);
    //8
    glColor3ub(68, 83, 2);
    circle(1,1,41,55);
    tree3(40,75);
    //9
    glColor3ub(68, 83, 2);
    circle(1,1,45,55);
    tree3(44,75);

    //10
    glColor3ub(68, 83, 2);
    circle(1.5,1.5,48.5,49);
    tree2(46,67);
    //11
    glColor3ub(68, 83, 2);
    circle(1,1,59,50);
    tree3(58,70);

    //12 other side of rail
    glColor3ub(68, 83, 2);
    circle(1,1,72,54);
    tree4(71,71);

    //second row 12
    glColor3ub(68, 83, 2);
    circle(1,1,2.5,35);
    tree3(1.5,55);

    //13
    glColor3ub(68, 83, 2);
    circle(1,1,7,40.5);
    tree4(6,57);
    //14
    glColor3ub(68, 83, 2);
    circle(1,1,12,37.5);
    tree4(11,54);
    //15
    glColor3ub(68, 83, 2);
    circle(1,1,17,32);
    tree3(16,52);
    //16
    glColor3ub(68, 83, 2);
    circle(1,1,20.5,38);
    tree3(19.5,58);

    //16
    glColor3ub(68, 83, 2);
    circle(1.5,1.5,14.5,27.5);
    tree2(12,45);
    //17
    glColor3ub(68, 83, 2);
    circle(1.5,1.5,22.5,27.5);
    tree2(20,45);
    //18
    glColor3ub(68, 83, 2);
    circle(1,1,31,37);
    tree4(30,54);

    //##
    glColor3ub(68, 83, 2);
   // circle(1,1,31,37);
    tree4(35,54);
    //##
    glColor3ub(68, 83, 2);
   // circle(1,1,31,37);
    tree4(42,54);

    //19
    glColor3ub(68, 83, 2);
    circle(1,1,47,38);
    tree3(46,58);
    //20
    glColor3ub(68, 83, 2);
    circle(1,1,56,33);
    tree3(55,53);
    //21
    glColor3ub(68, 83, 2);
    circle(1.5,1.5,58,27.5);
    tree2(55.5,45);
    //third row

    //22
    glColor3ub(68, 83, 2);
    circle(1,1,static_cast<GLfloat>(3.8),20.5);
    tree4(static_cast<GLfloat>(2.8),37);
    //23
    glColor3ub(68, 83, 2);
    circle(1,1,10,13.5);
    tree4(9,30);
    //24
    glColor3ub(68, 83, 2);
    circle(1.5,1.5,13,9);
    tree1(11,23);
    //25
    glColor3ub(68, 83, 2);
    circle(1,1,16,15);
    tree3(15,35);
    //26
    glColor3ub(68, 83, 2);
    circle(1.5,1.5,19.5,8.5);
    tree2(17,26);

    //27
    glColor3ub(68, 83, 2);
    circle(1.5,1.5,30,9);
    tree1(28,23);
    //28
    glColor3ub(68, 83, 2);
    circle(1,1,34,9);
    tree3(33,29);
    //28
    glColor3ub(68, 83, 2);
    circle(1,1,44,19);
    tree3(43,39);
    //29
    glColor3ub(68, 83, 2);
    circle(1.5,1.5,53.5,8.5);
    tree2(51,26);

    //30
    glColor3ub(68, 83, 2);
    circle(1,1,59,10);
    tree3(58,30);

    glColor3ub(68, 83, 2);
    circle(3.5,3,40,12);

    //body
    glColor3ub(68, 43, 2);
    glBegin(GL_QUADS);
        glVertex2d(39,36);
        glVertex2d(41,36);
        glVertex2d(42,11);
        glVertex2d(38,11);
    glEnd();
    //leaves
    glColor3ub(11, 70, 11);
    circle(3,4,35,37);
    circle(3,4,38,39);
    circle(3,4,40,37);
    circle(3,4,43,39);
    circle(3,4,44,35);
    circle(3,4,44,31);
    circle(3,4,40,33);
    circle(3,4,37,32);
    circle(3,4,35,34);

    //fruits
    glColor3ub(255, 165, 0);
    circle(.5,1,33,37);
    circle(.5,1,35,39);
    circle(.5,1,38,41);
    circle(.5,1,41,39);
    circle(.5,1,43,41);
    circle(.5,1,45,39);
    circle(.5,1,46,35);
    circle(.5,1,45,31);
    circle(.5,1,43,35);
    circle(.5,1,40,35);
    circle(.5,1,37,36);
    circle(.5,1,34,33);
    circle(.5,1,36,32);
    //fruits fall
    orangeFall();

    //diagonal Bird Move
    diagonalBirdMove();
}

void border()
{   glColor3f(1,1,1);
    glRectf(static_cast<GLfloat>(.8),1,99.5,1.5);
    glRectf(static_cast<GLfloat>(0.8),1,1,99.5);
    glRectf(static_cast<GLfloat>(0.8),99.5,99.5,99);
    glRectf(static_cast<GLfloat>(99.1),99.5,99.5,1);
}

#undef anim

} // namespace

const char* sceneName(Scene s)
{
    switch (s) {
    case Scene::Day:       return "day";
    case Scene::Rain:      return "rain";
    case Scene::Night:     return "night";
    case Scene::RainNight: return "rain_night";
    }
    return "?";
}

namespace {
// Advance a counter by `step` and wrap it back to `reset` once it passes `limit`.
// Steps are doubles on purpose: upstream wrote `x += .7;` on float variables,
// i.e. float + double rounded back to float, and keeping that arithmetic makes
// the port bit-identical to the original frame by frame.
inline void wrap(float& v, double step, double limit, double reset)
{
    v = static_cast<float>(v + step);
    if ((step < 0.0 && v < limit) || (step > 0.0 && v > limit)) v = static_cast<float>(reset);
}
} // namespace

void advance(AnimationState& s)
{
    wrap(s.train,      -0.5, -60.0, 0.0);      // trainMove
    wrap(s.bigBoat,    -0.5, -50.0, 0.0);      // bigboatMove
    wrap(s.smallBoats,  0.1,   0.5, 0.0);      // smallBoatMove
    wrap(s.cloud[0],    0.7, 200.0, -20.0);    // cloudMove
    wrap(s.cloud[1],    0.5, 200.0, -60.0);
    wrap(s.cloud[2],    0.5, 200.0, -80.0);
    wrap(s.cloud[3],    0.4, 200.0, -110.0);
    wrap(s.cloud[4],    0.5, 200.0, -130.0);   // upstream reset cloud[3] here (typo)
    wrap(s.birdLeftward,  -0.5, -100.0, 5.0);  // birMove
    wrap(s.birdRightward,  0.5,  100.0, 5.0);  //   (sic: restarts at +5, not -5)
    wrap(s.orange[0], -0.4, -29.0, 0.0);       // Orange
    wrap(s.orange[1], -0.5, -30.0, 0.0);
    wrap(s.orange[2], -0.6, -30.0, 0.0);
    wrap(s.orange[3], -0.5, -30.0, 0.0);
    wrap(s.orange[4], -0.3, -30.0, 0.0);
    wrap(s.orange[5], -1.0, -30.0, 0.0);
    wrap(s.planeLeftward,  -1.0, -100.0, 5.0); // planemove
    wrap(s.planeRightward,  1.0,  100.0, -7.0);
    s.diagBird1X = static_cast<float>(s.diagBird1X - 1.0);      // birdMove (diagonal)
    s.diagBird1Y = static_cast<float>(s.diagBird1Y + 0.8);
    if (s.diagBird1X < -60.0 && s.diagBird1Y > 90.0) s.diagBird1X = s.diagBird1Y = 0.f;
    wrap(s.diagBird2Y, 0.8,  80.0, 0.0);
    wrap(s.diagBird2X, 1.0, 100.0, 0.0);
    // Rain.  Upstream called its update routine — which decrements all twelve
    // layer offsets by one — once per layer *while drawing*, so the layers
    // fell 12 units per frame with a one-unit skew between successive layers
    // and only moved while a rainy scene was on screen.  The port moves each
    // layer by a fixed amount per tick and wraps modulo the layer spacing times
    // the layer count, which keeps the layers evenly spaced forever.
    for (float& r : s.rain) {
        r = static_cast<float>(r - kRainFallPerTick);
        if (r < -kRainPeriod) r = static_cast<float>(r + kRainPeriod);
    }
}

void setupProjection()
{
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, kWorldWidth, 0.0, kWorldHeight, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void render(Scene scene, const AnimationState& s)
{
    g_state = &s;
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1.f, 1.f, 1.f);

    const bool night = (scene == Scene::Night || scene == Scene::RainNight);
    const bool rain  = (scene == Scene::Rain  || scene == Scene::RainNight);

    // Upstream drew sky before road in day mode and road before sky otherwise;
    // the two do not overlap, so a single order is used for all scenes.
    if (night) nightsky(); else sky();
    road();
    homes();
    if (night) nightRiver(); else river();
    border();
    if (rain) rainFall();

    g_state = nullptr;
}

} // namespace dv
