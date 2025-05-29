#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <GL/freeglut.h>
#include "Game.h"

class InputHandler {
public:
    static void keyboard(unsigned char key, int x, int y, Game& game);
    static void keyboardUp(unsigned char key, int x, int y);
    static void specialKeyDown(int key, int x, int y);
    static void specialKeyUp(int key, int x, int y);
    static void mouse(int button, int state, int x, int y, Game& game);

    static bool isKeyPressed(int key) { return keys[key]; }
    static bool isSpecialKeyPressed(int key) { return specialKeys[key]; }

private:
    static bool keys[256];
    static bool specialKeys[256];
};

#endif