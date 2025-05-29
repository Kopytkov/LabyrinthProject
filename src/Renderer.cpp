#include "Renderer.h"
#include "Maze.h"
#include "Player.h"
#include <cmath>
#include "C:\LabyrinthProject\include\stb_image.h"

GLuint Renderer::wallTexture = 0;
GLuint Renderer::floorTexture = 0;
GLfloat Renderer::lightPos[] = { 0.0f, 10.0f, 0.0f, 1.0f };

void Renderer::initialize() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_FOG);
    GLfloat fogColor[4] = {0.5f, 0.5f, 0.5f, 1.0f};
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogf(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_START, 5.0f);
    glFogf(GL_FOG_END, 15.0f);

    GLfloat diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

    GLfloat ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);

    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.0f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0f);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 180.0f);
}

void Renderer::loadTexture(const std::string& filename, GLuint& textureID) {
    int width, height, channels;
    unsigned char* image = stbi_load(filename.c_str(), &width, &height, &channels, 0);
    if (!image) {
        printf("Не удалось загрузить текстуру: %s\n", filename.c_str());
        textureID = 0;
        return;
    }

    GLint maxTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    if (width > maxTextureSize || height > maxTextureSize) {
        printf("Ошибка: размер текстуры %dx%d превышает максимальный %d\n", width, height, maxTextureSize);
        stbi_image_free(image);
        textureID = 0;
        return;
    }

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, channels == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);
}

void Renderer::drawScene(bool showMiniMap) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, 800.0f / 600.0f, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float lookX = Player::getX() + sin(Player::getAngle());
    float lookZ = Player::getZ() + cos(Player::getAngle());
    gluLookAt(Player::getX(), Player::getY(), Player::getZ(), lookX, Player::getY(), lookZ, 0.0, 1.0, 0.0);

    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glDisable(GL_STENCIL_TEST);

    glColor3f(1.0f, 1.0f, 1.0f);
    if (floorTexture) {
        glBindTexture(GL_TEXTURE_2D, floorTexture);
    } else {
        glColor3f(0.5f, 0.5f, 0.5f);
    }
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-Maze::getInstance().getWidth() / 2, -1.0f, -Maze::getInstance().getHeight() / 2);
    glTexCoord2f(Maze::getInstance().getWidth() / 2.0f, 0.0f); glVertex3f(Maze::getInstance().getWidth() / 2, -1.0f, -Maze::getInstance().getHeight() / 2);
    glTexCoord2f(Maze::getInstance().getWidth() / 2.0f, Maze::getInstance().getHeight() / 2.0f); glVertex3f(Maze::getInstance().getWidth() / 2, -1.0f, Maze::getInstance().getHeight() / 2);
    glTexCoord2f(0.0f, Maze::getInstance().getHeight() / 2.0f); glVertex3f(-Maze::getInstance().getWidth() / 2, -1.0f, Maze::getInstance().getHeight() / 2);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);

    const std::vector<float>& walls = Maze::getInstance().getWalls();
    for (size_t i = 0; i < walls.size(); i += 4) {
        drawWall(walls[i], walls[i + 1], walls[i + 2], walls[i + 3]);
    }

    drawExit(Maze::getInstance().getExitX(), -0.5f, Maze::getInstance().getExitZ());

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_CULL_FACE);

    glCullFace(GL_BACK);
    glStencilFunc(GL_ALWAYS, 0, ~0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    for (size_t i = 0; i < walls.size(); i += 4) {
        drawShadowVolume(walls[i], walls[i + 1], walls[i + 2], walls[i + 3]);
    }

    glCullFace(GL_FRONT);
    glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
    for (size_t i = 0; i < walls.size(); i += 4) {
        drawShadowVolume(walls[i], walls[i + 1], walls[i + 2], walls[i + 3]);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_FALSE);
    glStencilFunc(GL_NOTEQUAL, 0, ~0);
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
    glBegin(GL_QUADS);
    glVertex3f(-Maze::getInstance().getWidth() / 2, -0.99f, -Maze::getInstance().getHeight() / 2);
    glVertex3f(Maze::getInstance().getWidth() / 2, -0.99f, -Maze::getInstance().getHeight() / 2);
    glVertex3f(Maze::getInstance().getWidth() / 2, -0.99f, Maze::getInstance().getHeight() / 2);
    glVertex3f(-Maze::getInstance().getWidth() / 2, -0.99f, Maze::getInstance().getHeight() / 2);
    glEnd();

    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    if (showMiniMap) {
        drawMiniMap();
    }

    glutSwapBuffers();
}

void Renderer::drawMenu() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, Game::instance->getWindowWidth(), 0, Game::instance->getWindowHeight(), -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);

    float windowWidth = Game::instance->getWindowWidth();
    float windowHeight = Game::instance->getWindowHeight();

    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(0.375f * windowWidth, 0.75f * windowHeight, "Choose Difficulty");

    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(0.375f * windowWidth, 0.583f * windowHeight);
    glVertex2f(0.625f * windowWidth, 0.583f * windowHeight);
    glVertex2f(0.625f * windowWidth, 0.666f * windowHeight);
    glVertex2f(0.375f * windowWidth, 0.666f * windowHeight);
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(0.4375f * windowWidth, 0.616f * windowHeight, "Easy");

    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(0.375f * windowWidth, 0.458f * windowHeight);
    glVertex2f(0.625f * windowWidth, 0.458f * windowHeight);
    glVertex2f(0.625f * windowWidth, 0.541f * windowHeight);
    glVertex2f(0.375f * windowWidth, 0.541f * windowHeight);
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(0.425f * windowWidth, 0.491f * windowHeight, "Medium");

    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(0.375f * windowWidth, 0.333f * windowHeight);
    glVertex2f(0.625f * windowWidth, 0.333f * windowHeight);
    glVertex2f(0.625f * windowWidth, 0.416f * windowHeight);
    glVertex2f(0.375f * windowWidth, 0.416f * windowHeight);
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(0.4375f * windowWidth, 0.366f * windowHeight, "Hard");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glutSwapBuffers();
}

void Renderer::drawWinScreen(int activeMessage) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, Game::instance->getWindowWidth(), 0, Game::instance->getWindowHeight(), -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);

    float windowWidth = Game::instance->getWindowWidth();
    float windowHeight = Game::instance->getWindowHeight();

    glColor3f(0.0f, 1.0f, 0.0f);
    glRasterPos2f(0.375f * windowWidth, 0.666f * windowHeight);
    const char* winMessage = "YOU WIN!";
    for (const char* c = winMessage; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
    }

    glColor3f(activeMessage == 0 ? 1.0f : 0.5f, 1.0f, 0.0f);
    drawText(0.375f * windowWidth, 0.583f * windowHeight, "Start again?");

    glColor3f(activeMessage == 1 ? 1.0f : 0.5f, 1.0f, 0.0f);
    drawText(0.375f * windowWidth, 0.5f * windowHeight, "Exit the game?");

    glColor3f(activeMessage == 2 ? 1.0f : 0.5f, 1.0f, 0.0f);
    drawText(0.375f * windowWidth, 0.416f * windowHeight, "Go back to the menu?");

    if (activeMessage != -1) {
        float buttonY = (activeMessage == 0 ? 0.533f : (activeMessage == 1 ? 0.45f : 0.366f)) * windowHeight;
        glColor3f(0.0f, 1.0f, 0.0f);
        glBegin(GL_QUADS);
        glVertex2f(0.4375f * windowWidth, buttonY);
        glVertex2f(0.5f * windowWidth, buttonY);
        glVertex2f(0.5f * windowWidth, buttonY + 0.033f * windowHeight);
        glVertex2f(0.4375f * windowWidth, buttonY + 0.033f * windowHeight);
        glEnd();
        glColor3f(0.0f, 0.0f, 0.0f);
        drawText(0.45f * windowWidth, buttonY + 0.008f * windowHeight, "YES");

        glColor3f(1.0f, 0.0f, 0.0f);
        glBegin(GL_QUADS);
        glVertex2f(0.5125f * windowWidth, buttonY);
        glVertex2f(0.575f * windowWidth, buttonY);
        glVertex2f(0.575f * windowWidth, buttonY + 0.033f * windowHeight);
        glVertex2f(0.5125f * windowWidth, buttonY + 0.033f * windowHeight);
        glEnd();
        glColor3f(0.0f, 0.0f, 0.0f);
        drawText(0.525f * windowWidth, buttonY + 0.008f * windowHeight, "NO");
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glutSwapBuffers();
}

void Renderer::reshape(int w, int h, GameState state) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (state == GameState::MENU || state == GameState::WIN) {
        glOrtho(0, w, 0, h, -1, 1);
    } else {
        gluPerspective(45.0f, (float)w / h, 0.1f, 100.0f);
    }
    glMatrixMode(GL_MODELVIEW);
}

void Renderer::drawText(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
}

void Renderer::drawWall(float x, float z, float width, float height, bool shadowPass) {
    if (!shadowPass) {
        glColor3f(1.0f, 1.0f, 1.0f);
        if (wallTexture) {
            glBindTexture(GL_TEXTURE_2D, wallTexture);
        } else {
            glColor3f(1.0f, 1.0f, 0.0f);
        }
    }

    glBegin(GL_QUADS);

    glNormal3f(0.0f, 0.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x, -1.0f, z);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x,  1.0f, z);
    glTexCoord2f(width / 2.0f, 1.0f); glVertex3f(x + width,  1.0f, z);
    glTexCoord2f(width / 2.0f, 0.0f); glVertex3f(x + width, -1.0f, z);

    glNormal3f(0.0f, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x, -1.0f, z + height);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x,  1.0f, z + height);
    glTexCoord2f(width / 2.0f, 1.0f); glVertex3f(x + width,  1.0f, z + height);
    glTexCoord2f(width / 2.0f, 0.0f); glVertex3f(x + width, -1.0f, z + height);

    glNormal3f(-1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x, -1.0f, z);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x,  1.0f, z);
    glTexCoord2f(height / 2.0f, 1.0f); glVertex3f(x,  1.0f, z + height);
    glTexCoord2f(height / 2.0f, 0.0f); glVertex3f(x, -1.0f, z + height);

    glNormal3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x + width, -1.0f, z);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x + width,  1.0f, z);
    glTexCoord2f(height / 2.0f, 1.0f); glVertex3f(x + width,  1.0f, z + height);
    glTexCoord2f(height / 2.0f, 0.0f); glVertex3f(x + width, -1.0f, z + height);

    glNormal3f(0.0f, -1.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x, -1.0f, z);
    glTexCoord2f(width / 2.0f, 0.0f); glVertex3f(x + width, -1.0f, z);
    glTexCoord2f(width / 2.0f, height / 2.0f); glVertex3f(x + width, -1.0f, z + height);
    glTexCoord2f(0.0f, height / 2.0f); glVertex3f(x, -1.0f, z + height);

    glNormal3f(0.0f, 1.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x,  1.0f, z);
    glTexCoord2f(width / 2.0f, 0.0f); glVertex3f(x + width,  1.0f, z);
    glTexCoord2f(width / 2.0f, height / 2.0f); glVertex3f(x + width,  1.0f, z + height);
    glTexCoord2f(0.0f, height / 2.0f); glVertex3f(x,  1.0f, z + height);

    glEnd();

    if (!shadowPass) {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void Renderer::drawShadowVolume(float x, float z, float width, float height) {
    float shadowY = -1.0f;
    float lightY = lightPos[1];
    float lightX = lightPos[0];
    float lightZ = lightPos[2];

    glBegin(GL_QUADS);

    glVertex3f(x, 1.0f, z);
    glVertex3f(x + width, 1.0f, z);
    float projX1 = x + (lightX - x) * (1.0f - shadowY) / (lightY - shadowY);
    float projX2 = (x + width) + (lightX - (x + width)) * (1.0f - shadowY) / (lightY - shadowY);
    float projZ = z + (lightZ - z) * (1.0f - shadowY) / (lightY - shadowY);
    glVertex3f(projX2, shadowY, projZ);
    glVertex3f(projX1, shadowY, projZ);

    glVertex3f(x, 1.0f, z + height);
    glVertex3f(x + width, 1.0f, z + height);
    projX1 = x + (lightX - x) * (1.0f - shadowY) / (lightY - shadowY);
    projX2 = (x + width) + (lightX - (x + width)) * (1.0f - shadowY) / (lightY - shadowY);
    projZ = (z + height) + (lightZ - (z + height)) * (1.0f - shadowY) / (lightY - shadowY);
    glVertex3f(projX2, shadowY, projZ);
    glVertex3f(projX1, shadowY, projZ);

    glVertex3f(x, 1.0f, z);
    glVertex3f(x, 1.0f, z + height);
    projX1 = x + (lightX - x) * (1.0f - shadowY) / (lightY - shadowY);
    projZ = z + (lightZ - z) * (1.0f - shadowY) / (lightY - shadowY);
    float projZ2 = (z + height) + (lightZ - (z + height)) * (1.0f - shadowY) / (lightY - shadowY);
    glVertex3f(projX1, shadowY, projZ2);
    glVertex3f(projX1, shadowY, projZ);

    glVertex3f(x + width, 1.0f, z);
    glVertex3f(x + width, 1.0f, z + height);
    projX2 = (x + width) + (lightX - (x + width)) * (1.0f - shadowY) / (lightY - shadowY);
    projZ = z + (lightZ - z) * (1.0f - shadowY) / (lightY - shadowY);
    projZ2 = (z + height) + (lightZ - (z + height)) * (1.0f - shadowY) / (lightY - shadowY);
    glVertex3f(projX2, shadowY, projZ2);
    glVertex3f(projX2, shadowY, projZ);

    glEnd();
}

void Renderer::drawExit(float x, float y, float z) {
    glDisable(GL_LIGHTING);
    glBindTexture(GL_TEXTURE_2D, 0);
    glColor3f(1.0f, 0.0f, 0.0f);
    glPushMatrix();
    glTranslatef(x, y, z);
    glutSolidCube(0.5f);
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

void Renderer::drawMiniMap() {
    glPushMatrix();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, Game::instance->getWindowWidth(), 0, Game::instance->getWindowHeight(), -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);

    float windowWidth = Game::instance->getWindowWidth();
    float windowHeight = Game::instance->getWindowHeight();
    float mapScale = 0.25f * windowWidth / Maze::getInstance().getWidth();
    float mapY = -0.165f * windowHeight / Maze::getInstance().getHeight(); 

    glColor3f(1.0f, 1.0f, 1.0f);
    const std::vector<float>& walls = Maze::getInstance().getWalls();
    for (size_t i = 0; i < walls.size(); i += 4) {
        float x = walls[i] * mapScale + 0.125f * windowWidth;
        float z = (Maze::getInstance().getHeight() - walls[i + 1]) * mapScale + mapY;
        float w = walls[i + 2] * mapScale;
        float h = walls[i + 3] * mapScale;
        glBegin(GL_QUADS);
        glVertex2f(x, z);
        glVertex2f(x + w, z);
        glVertex2f(x + w, z - h);
        glVertex2f(x, z - h);
        glEnd();
    }

    glColor3f(0.0f, 1.0f, 0.0f);
    float playerX = Player::getX() * mapScale + 0.125f * windowWidth;
    float playerZ = (Maze::getInstance().getHeight() - Player::getZ()) * mapScale + mapY;
    float arrowSize = 0.0125f * windowWidth;  // 10 при 800

    float tipX = playerX + arrowSize * sin(Player::getAngle());
    float tipZ = playerZ - arrowSize * cos(Player::getAngle());
    float base1X = playerX + arrowSize * 0.5f * sin(Player::getAngle() + 2.5f);
    float base1Z = playerZ - arrowSize * 0.5f * cos(Player::getAngle() + 2.5f);
    float base2X = playerX + arrowSize * 0.5f * sin(Player::getAngle() - 2.5f);
    float base2Z = playerZ - arrowSize * 0.5f * cos(Player::getAngle() - 2.5f);

    glBegin(GL_TRIANGLES);
    glVertex2f(tipX, tipZ);
    glVertex2f(base1X, base1Z);
    glVertex2f(base2X, base2Z);
    glEnd();

    glColor3f(1.0f, 0.0f, 0.0f);
    float exitMapX = Maze::getInstance().getExitX() * mapScale + 0.125f * windowWidth;
    float exitMapZ = (Maze::getInstance().getHeight() - Maze::getInstance().getExitZ()) * mapScale + mapY;
    glPointSize(5.0f);
    glBegin(GL_POINTS);
    glVertex2f(exitMapX, exitMapZ);
    glEnd();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}