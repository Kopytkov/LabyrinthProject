#ifndef RENDERER_H
#define RENDERER_H

#include <GL/freeglut.h>
#include <string>  // Добавлено
#include "Game.h"

class Renderer {
public:
    static void initialize();
    static void loadTexture(const std::string& filename, GLuint& textureID);
    static void drawScene(bool showMiniMap);
    static void drawMenu();
    static void drawWinScreen(int activeMessage);
    static void reshape(int w, int h, GameState state);

    static GLuint wallTexture;
    static GLuint floorTexture;
    static GLfloat lightPos[];

private:
    static void drawText(float x, float y, const char* text);
    static void drawWall(float x, float z, float width, float height, bool shadowPass = false);
    static void drawShadowVolume(float x, float z, float width, float height);
    static void drawExit(float x, float y, float z);
    static void drawMiniMap();
};

#endif