#ifndef PLAYER_H
#define PLAYER_H

#include "Game.h"

class Player {
public:
    static void update(Game& game);

    static float getX() { return x; }
    static float getY() { return y; }
    static float getZ() { return z; }
    static float getAngle() { return angle; }

    static void setX(float newX) { x = newX; }
    static void setZ(float newZ) { z = newZ; }
    static void setAngle(float newAngle) { angle = newAngle; }
    static bool checkCollision(float newX, float newZ, float x, float z, float width, float height);

private:
    static float x, y, z;
    static float angle;
};

#endif