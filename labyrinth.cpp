#include <GL/freeglut.h>
#include <math.h>
#include <stdio.h>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Переменные для камеры и цвета фона
float camX = 0.0f, camY = 0.0f, camZ = 0.0f;
float angle = 0.0f; // Начальный угол камеры — вверх
float bgR = 0.0f, bgG = 0.0f, bgB = 0.0f;
float exitX = 0.0f, exitZ = 0.0f;

// Размеры игрового поля
float worldWidth = 20.0f, worldHeight = 20.0f;

// Вектор для хранения стен (x, z, длина, толщина)
std::vector<float> walls;

// Позиция света
GLfloat lightPos[] = { 0.0f, 10.0f, 0.0f, 1.0f };

// Идентификаторы текстур
GLuint wallTexture = 0, floorTexture = 0;

// Массивы для отслеживания состояния клавиш
bool keys[256] = {false};         // Обычные клавиши (Q, E, A, D)
bool specialKeys[256] = {false};  // Специальные клавиши (стрелки)

// Состояние игры
enum GameState { MENU, PLAYING, WIN };
GameState gameState = MENU;

// Состояние мини-карты (изначально выключена)
bool showMiniMap = false;

// Переменная для отслеживания выбора на экране победы (0 - "Start again?", 1 - "Exit the game?")
int activeMessage = -1; // -1 — нет активного сообщения, 0 — "Start again?", 1 — "Exit"

// Прототипы функций
void display();

void loadTexture(const char* filename, GLuint& textureID) {
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (!image) {
        printf("Не удалось загрузить текстуру: %s (файл отсутствует или повреждён)\n", filename);
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

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        printf("Ошибка OpenGL перед glTexImage2D: %d\n", error);
        stbi_image_free(image);
        glDeleteTextures(1, &textureID);
        textureID = 0;
        return;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, channels == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, image);
    error = glGetError();
    if (error != GL_NO_ERROR) {
        printf("Ошибка OpenGL в glTexImage2D для %s: %d\n", filename, error);
        stbi_image_free(image);
        glDeleteTextures(1, &textureID);
        textureID = 0;
        return;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    error = glGetError();
    if (error != GL_NO_ERROR) {
        printf("Ошибка OpenGL в glTexParameteri для %s: %d\n", filename, error);
        stbi_image_free(image);
        glDeleteTextures(1, &textureID);
        textureID = 0;
        return;
    }

    printf("Текстура загружена: %s\n", filename);
    stbi_image_free(image);
}

void init() {
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

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        printf("Ошибка OpenGL в init: %d\n", error);
    }
}

void loadMazeFromImage(const char* filename) {
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 3);

    if (!image) {
        printf("Ошибка загрузки изображения: %s\n", filename);
        return;
    }

    walls.clear();

    float aspectRatio = (float)width / height;
    worldWidth = 20.0f;
    worldHeight = worldWidth / aspectRatio;
    float scaleX = worldWidth / width;
    float scaleZ = worldHeight / height;
    float wallThickness = 0.5f;

    // Ищем старт в нижней строке (первая белая точка слева)
    bool startFound = false;
    int startX = 0;
    for (int x = 0; x < width && !startFound; x++) {
        int index = ((height - 1) * width + x) * 3; // Нижняя строка
        if (image[index] == 255 && image[index + 1] == 255 && image[index + 2] == 255) {
            startX = x;
            startFound = true;
        }
    }
    if (startFound) {
        int shiftedX = startX + 2;
        if (shiftedX >= width) shiftedX = width - 1;
        camX = (shiftedX * scaleX) - (worldWidth / 2) + (scaleX / 2);
        camZ = (worldHeight / 2) - (scaleZ / 2); // Старт внизу (положительный Z)
        angle = M_PI; // Смотрим вниз (на 180 градусов от оси Z)
    } else {
        printf("Стартовая позиция не найдена!\n");
        camX = -(worldWidth / 2) + 1.0f;
        camZ = (worldHeight / 2) - 1.0f; // По умолчанию внизу
        angle = M_PI;
    }

    // Ищем финиш в верхней строке (последняя белая точка справа)
    bool exitFound = false;
    int exitXPixel = 0;
    for (int x = width - 1; x >= 0 && !exitFound; x--) {
        int index = x * 3; // Верхняя строка
        if (image[index] == 255 && image[index + 1] == 255 && image[index + 2] == 255) {
            exitXPixel = x;
            exitFound = true;
        }
    }
    if (exitFound) {
        int shiftedX = exitXPixel - 2;
        if (shiftedX < 0) shiftedX = 0;
        exitX = (shiftedX * scaleX) - (worldWidth / 2) + (scaleX / 2);
        exitZ = -(worldHeight / 2) + (scaleZ / 2); // Финиш вверху (отрицательный Z)
    } else {
        printf("Выход не найден!\n");
        exitX = (worldWidth / 2) - 1.0f;
        exitZ = -(worldHeight / 2) + 1.0f; // По умолчанию вверху
    }

    std::vector<bool> visited(width * height, false);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = (y * width + x) * 3;
            if (!visited[y * width + x] && image[index] == 0 && image[index + 1] == 0 && image[index + 2] == 0) {
                int wallWidth = 1;
                for (int wx = x + 1; wx < width; wx++) {
                    int wIndex = (y * width + wx) * 3;
                    if (image[wIndex] == 0 && image[wIndex + 1] == 0 && image[wIndex + 2] == 0) {
                        wallWidth++;
                        visited[y * width + wx] = true;
                    } else {
                        break;
                    }
                }

                float x1 = (x * scaleX) - (worldWidth / 2);
                float z1 = (y * scaleZ) - (worldHeight / 2);
                float wallLength = wallWidth * scaleX;

                walls.push_back(x1);
                walls.push_back(z1);
                walls.push_back(wallLength);
                walls.push_back(wallThickness);
            }
        }
    }

    printf("Карта загружена: %s, стен: %zu\n", filename, walls.size() / 4);
    stbi_image_free(image);
}

void drawWall(float x, float z, float width, float height, bool shadowPass = false) {
    if (!shadowPass) {
        glColor3f(1.0f, 1.0f, 1.0f);
        if (wallTexture) {
            glBindTexture(GL_TEXTURE_2D, wallTexture);
        } else {
            glColor3f(1.0f, 1.0f, 0.0f);
            printf("wallTexture не загружена\n");
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

void drawShadowVolume(float x, float z, float width, float height) {
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

void drawExit(float x, float y, float z) {
    glDisable(GL_LIGHTING);
    glBindTexture(GL_TEXTURE_2D, 0);
    glColor3f(1.0f, 0.0f, 0.0f);
    glPushMatrix();
    glTranslatef(x, y, z);
    glutSolidCube(0.5f);
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

bool checkCollision(float newX, float newZ, float x, float z, float width, float height) {
    float minX = x - 0.2f;
    float maxX = x + width + 0.2f;
    float minZ = z - 0.2f;
    float maxZ = z + height + 0.2f;
    return (newX >= minX && newX <= maxX && newZ >= minZ && newZ <= maxZ);
}

void drawText(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
}

void drawMenu() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 800, 0, 600, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);

    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(300, 450, "Choose Difficulty");

    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(300, 350);
    glVertex2f(500, 350);
    glVertex2f(500, 400);
    glVertex2f(300, 400);
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(350, 370, "Easy");

    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(300, 275);
    glVertex2f(500, 275);
    glVertex2f(500, 325);
    glVertex2f(300, 325);
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(340, 295, "Medium");

    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(300, 200);
    glVertex2f(500, 200);
    glVertex2f(500, 250);
    glVertex2f(300, 250);
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(350, 220, "Hard");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glutSwapBuffers();
}

void mouseFunc(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        y = 600 - y; // Переводим координаты Y (GLUT считает сверху вниз)

        if (gameState == MENU) {
            if (x >= 300 && x <= 500 && y >= 350 && y <= 400) {
                loadMazeFromImage("C:/LabyrinthProject/maze_easy.png");
                if (!walls.empty()) {
                    loadTexture("C:/LabyrinthProject/wall_texture.png", wallTexture);
                    loadTexture("C:/LabyrinthProject/floor_texture.png", floorTexture);
                    init();
                    gameState = PLAYING;
                    glutDisplayFunc(display);
                    glutPostRedisplay();
                } else {
                    printf("Не удалось загрузить стены для maze_easy.png\n");
                }
            }
            else if (x >= 300 && x <= 500 && y >= 275 && y <= 325) {
                loadMazeFromImage("C:/LabyrinthProject/maze_medium.png");
                if (!walls.empty()) {
                    loadTexture("C:/LabyrinthProject/wall_texture.png", wallTexture);
                    loadTexture("C:/LabyrinthProject/floor_texture.png", floorTexture);
                    init();
                    gameState = PLAYING;
                    glutDisplayFunc(display);
                    glutPostRedisplay();
                } else {
                    printf("Не удалось загрузить стены для maze_medium.png\n");
                }
            }
            else if (x >= 300 && x <= 500 && y >= 200 && y <= 250) {
                loadMazeFromImage("C:/LabyrinthProject/maze_hard.png");
                if (!walls.empty()) {
                    loadTexture("C:/LabyrinthProject/wall_texture.png", wallTexture);
                    loadTexture("C:/LabyrinthProject/floor_texture.png", floorTexture);
                    init();
                    gameState = PLAYING;
                    glutDisplayFunc(display);
                    glutPostRedisplay();
                } else {
                    printf("Не удалось загрузить стены для maze_hard.png\n");
                }
            }
        }
        else if (gameState == WIN) {
            // Клик по "Start again?" (Y: 340-360)
            if (x >= 300 && x <= 500 && y >= 340 && y <= 360) {
                activeMessage = 0; // Активируем "Start again?"
                glutPostRedisplay();
            }
            // Клик по "Exit the game?" (Y: 290-310)
            else if (x >= 300 && x <= 500 && y >= 290 && y <= 310) {
                activeMessage = 1; // Активируем "Exit"
                glutPostRedisplay();
            }
            // Клик по "Go back to the menu?" (Y: 240-260)
            else if (x >= 300 && x <= 500 && y >= 240 && y <= 260) {
                activeMessage = 2; // Активируем "Go back to the menu?"
                glutPostRedisplay();
            }
            // Клик по кнопкам "YES" или "NO"
            if (activeMessage == 0) { // Для "Start again?"
                if (x >= 350 && x <= 400 && y >= 320 && y <= 340) { // YES
                    gameState = MENU;
                    showMiniMap = false;
                    activeMessage = -1;
                    glutDisplayFunc(display);
                    glutPostRedisplay();
                }
                else if (x >= 410 && x <= 460 && y >= 320 && y <= 340) { // NO
                    activeMessage = -1;
                    glutPostRedisplay();
                }
            }
            else if (activeMessage == 1) { // Для "Exit"
                if (x >= 350 && x <= 400 && y >= 270 && y <= 290) { // YES
                    exit(0);
                }
                else if (x >= 410 && x <= 460 && y >= 270 && y <= 290) { // NO
                    activeMessage = -1;
                    glutPostRedisplay();
                }
            }
            else if (activeMessage == 2) { // Для "Go back to the menu?"
                if (x >= 350 && x <= 400 && y >= 220 && y <= 240) { // YES
                    gameState = MENU;
                    showMiniMap = false;
                    activeMessage = -1;
                    glutDisplayFunc(display);
                    glutPostRedisplay();
                }
                else if (x >= 410 && x <= 460 && y >= 220 && y <= 240) { // NO
                    activeMessage = -1;
                    glutPostRedisplay();
                }
            }
        }
    }
}

void drawMiniMap() {
    glPushMatrix();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 800, 0, 600, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);

    float mapScale = 200.0f / worldWidth;

    // Отрисовка стен
    glColor3f(1.0f, 1.0f, 1.0f);
    for (size_t i = 0; i < walls.size(); i += 4) {
        float x = walls[i] * mapScale + 100;
        float z = (worldHeight - walls[i + 1]) * mapScale + 50;
        float w = walls[i + 2] * mapScale;
        float h = walls[i + 3] * mapScale;
        glBegin(GL_QUADS);
        glVertex2f(x, z);
        glVertex2f(x + w, z);
        glVertex2f(x + w, z - h);
        glVertex2f(x, z - h);
        glEnd();
    }

    // Отрисовка игрока (стрелки)
    glColor3f(0.0f, 1.0f, 0.0f);
    float playerX = camX * mapScale + 100;
    float playerZ = (worldHeight - camZ) * mapScale + 50;
    float arrowSize = 10.0f;

    float tipX = playerX + arrowSize * sin(angle);
    float tipZ = playerZ - arrowSize * cos(angle);
    float base1X = playerX + arrowSize * 0.5f * sin(angle + 2.5f);
    float base1Z = playerZ - arrowSize * 0.5f * cos(angle + 2.5f);
    float base2X = playerX + arrowSize * 0.5f * sin(angle - 2.5f);
    float base2Z = playerZ - arrowSize * 0.5f * cos(angle - 2.5f);

    glBegin(GL_TRIANGLES);
    glVertex2f(tipX, tipZ);
    glVertex2f(base1X, base1Z);
    glVertex2f(base2X, base2Z);
    glEnd();

    // Отрисовка выхода
    glColor3f(1.0f, 0.0f, 0.0f);
    float exitMapX = exitX * mapScale + 100;
    float exitMapZ = (worldHeight - exitZ) * mapScale + 50;
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

void drawWinScreen() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 800, 0, 600, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);

    // Большое сообщение "YOU WIN!"
    glColor3f(0.0f, 1.0f, 0.0f); // Зелёный цвет
    glRasterPos2f(300, 400);
    const char* winMessage = "YOU WIN!";
    for (const char* c = winMessage; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c); // Больший шрифт
    }

    // Сообщение "Start again?"
    glColor3f(activeMessage == 0 ? 1.0f : 0.5f, 1.0f, 0.0f); // Жёлтый, если активно
    drawText(300, 350, "Start again?");

    // Сообщение "Exit the game?"
    glColor3f(activeMessage == 1 ? 1.0f : 0.5f, 1.0f, 0.0f); // Жёлтый, если активно
    drawText(300, 300, "Exit the game?");

    // Сообщение "Go back to the menu?"
    glColor3f(activeMessage == 2 ? 1.0f : 0.5f, 1.0f, 0.0f); // Жёлтый, если активно
    drawText(300, 250, "Go back to the menu?");

    // Кнопки "YES" и "NO" появляются только для активного сообщения
    if (activeMessage != -1) {
        int buttonY = (activeMessage == 0 ? 320 : (activeMessage == 1 ? 270 : 220)); // Y для кнопок
        // Кнопка "YES"
        glColor3f(0.0f, 1.0f, 0.0f); // Зелёный
        glBegin(GL_QUADS);
        glVertex2f(350, buttonY);
        glVertex2f(400, buttonY);
        glVertex2f(400, buttonY + 20);
        glVertex2f(350, buttonY + 20);
        glEnd();
        glColor3f(0.0f, 0.0f, 0.0f); // Чёрный текст
        drawText(360, buttonY + 5, "YES");

        // Кнопка "NO"
        glColor3f(1.0f, 0.0f, 0.0f); // Красный
        glBegin(GL_QUADS);
        glVertex2f(410, buttonY);
        glVertex2f(460, buttonY);
        glVertex2f(460, buttonY + 20);
        glVertex2f(410, buttonY + 20);
        glEnd();
        glColor3f(0.0f, 0.0f, 0.0f); // Чёрный текст
        drawText(420, buttonY + 5, "NO");
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glutSwapBuffers();
}

void display() {
    if (gameState == MENU) {
        drawMenu();
        return;
    }
    if (gameState == WIN) {
        drawWinScreen();
        return;
    }

    glClearColor(bgR, bgG, bgB, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, 800.0f / 600.0f, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float lookX = camX + sin(angle);
    float lookZ = camZ + cos(angle);
    gluLookAt(camX, camY, camZ, lookX, camY, lookZ, 0.0, 1.0, 0.0);

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
        printf("floorTexture не загружена\n");
    }
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-worldWidth / 2, -1.0f, -worldHeight / 2);
    glTexCoord2f(worldWidth / 2.0f, 0.0f); glVertex3f(worldWidth / 2, -1.0f, -worldHeight / 2);
    glTexCoord2f(worldWidth / 2.0f, worldHeight / 2.0f); glVertex3f(worldWidth / 2, -1.0f, worldHeight / 2);
    glTexCoord2f(0.0f, worldHeight / 2.0f); glVertex3f(-worldWidth / 2, -1.0f, worldHeight / 2);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);

    for (size_t i = 0; i < walls.size(); i += 4) {
        drawWall(walls[i], walls[i + 1], walls[i + 2], walls[i + 3], false);
    }

    drawExit(exitX, -0.5f, exitZ);

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
    glVertex3f(-worldWidth / 2, -0.99f, -worldHeight / 2);
    glVertex3f(worldWidth / 2, -0.99f, -worldHeight / 2);
    glVertex3f(worldWidth / 2, -0.99f, worldHeight / 2);
    glVertex3f(-worldWidth / 2, -0.99f, worldHeight / 2);
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

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        printf("Ошибка OpenGL в display: %d\n", error);
    }
}

void update() {
    if (gameState == MENU || gameState == WIN) return;

    float speed = 0.005f;
    float rotSpeed = 0.005f;
    float newCamX = camX, newCamZ = camZ;

    if (specialKeys[GLUT_KEY_UP] || keys['w'] || keys['W'] || keys[214] || keys[246]) {
        newCamZ += speed * cos(angle);
        newCamX += speed * sin(angle);
    }
    if (specialKeys[GLUT_KEY_DOWN] || keys['s'] || keys['S'] || keys[219] || keys[251]) {
        newCamZ -= speed * cos(angle);
        newCamX -= speed * sin(angle);
    }
    if (specialKeys[GLUT_KEY_LEFT] || keys['a'] || keys['A'] || keys[212] || keys[244]) {
        newCamX += speed * cos(angle);
        newCamZ -= speed * sin(angle);
    }
    if (specialKeys[GLUT_KEY_RIGHT] || keys['d'] || keys['D'] || keys[194] || keys[226]) {
        newCamX -= speed * cos(angle);
        newCamZ += speed * sin(angle);
    }

    if (keys['q'] || keys['Q'] || keys[233] || keys[201]) {
        angle += rotSpeed;
    }
    if (keys['e'] || keys['E'] || keys[243] || keys[211]) {
        angle -= rotSpeed;
    }

    bool collision = false;
    for (size_t i = 0; i < walls.size(); i += 4) {
        if (checkCollision(newCamX, newCamZ, walls[i], walls[i + 1], walls[i + 2], walls[i + 3])) {
            collision = true;
            break;
        }
    }

    if (!collision) {
        camX = newCamX;
        camZ = newCamZ;
    }

    if (fabs(camX - exitX) < 0.5f && fabs(camZ - exitZ) < 0.5f) {
        gameState = WIN; // Переход в состояние победы
        printf("You reached the exit!\n");
    }

    glutPostRedisplay();
}

void specialKeyDown(int key, int x, int y) {
    specialKeys[key] = true;
}

void specialKeyUp(int key, int x, int y) {
    specialKeys[key] = false;
}

void keyboard(unsigned char key, int x, int y) {
    keys[key] = true;
    if (gameState == PLAYING) {
        if (key == 'm' || key == 'M') { // Переключение мини-карты
            showMiniMap = !showMiniMap;
            glutPostRedisplay();
        }
    }
    else if (gameState == WIN && activeMessage != -1) {
        if (key == 'y' || key == 'Y') { // "Yes"
            if (activeMessage == 0) { // Start again
                gameState = MENU;
                showMiniMap = false;
                activeMessage = -1;
                glutDisplayFunc(display);
            }
            else if (activeMessage == 1) { // Exit
                exit(0);
            }
            else if (activeMessage == 2) { // Go back to the menu
                gameState = MENU;
                showMiniMap = false;
                activeMessage = -1;
                glutDisplayFunc(display);
            }
            glutPostRedisplay();
        }
        else if (key == 'n' || key == 'N') { // "No"
            activeMessage = -1; // Сбрасываем выбор
            glutPostRedisplay();
        }
    }
}

void keyboardUp(unsigned char key, int x, int y) {
    keys[key] = false;
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (gameState == MENU || gameState == WIN) {
        glOrtho(0, 800, 0, 600, -1, 1);
    } else {
        gluPerspective(45.0f, (float)w / h, 0.1f, 100.0f);
    }
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
    glutInitWindowSize(800, 600);
    glutCreateWindow("3D Labyrinth with Shadows and Textures");

    init();

    glutDisplayFunc(drawMenu);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialKeyDown);
    glutSpecialUpFunc(specialKeyUp);
    glutMouseFunc(mouseFunc);
    glutIdleFunc(update);

    glutMainLoop();
    return 0;
}