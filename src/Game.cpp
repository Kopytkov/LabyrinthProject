#include "Game.h"
#include "Renderer.h"
#include "Maze.h"
#include "Player.h"
#include "InputHandler.h"
#include <string>

Game* Game::instance = nullptr;

Game::Game() : state(GameState::MENU), showMiniMap(false), activeMessage(-1), currentLevel(""), windowWidth(800), windowHeight(600) {  // Инициализация размеров
    instance = this;
}

void Game::initialize(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
    glutInitWindowSize(800, 600);
    glutCreateWindow("3D Labyrinth with Shadows and Textures");

    Renderer::initialize();

    glutDisplayFunc(displayCallback);
    glutReshapeFunc(reshapeCallback);
    glutKeyboardFunc(keyboardCallback);
    glutKeyboardUpFunc(keyboardUpCallback);
    glutSpecialFunc(specialKeyDownCallback);
    glutSpecialUpFunc(specialKeyUpCallback);
    glutMouseFunc(mouseCallback);
    glutIdleFunc(updateCallback);
}

void Game::run() {
    glutMainLoop();
}

void Game::displayCallback() {
    if (instance->getState() == GameState::MENU) {
        Renderer::drawMenu();
    } else if (instance->getState() == GameState::WIN) {
        Renderer::drawWinScreen(instance->getActiveMessage());
    } else {
        Renderer::drawScene(instance->isMiniMapShown());
    }
}

void Game::reshapeCallback(int w, int h) {
    instance->windowWidth = w;   // Обновляем ширину
    instance->windowHeight = h;  // Обновляем высоту
    Renderer::reshape(w, h, instance->getState());
}

void Game::keyboardCallback(unsigned char key, int x, int y) {
    InputHandler::keyboard(key, x, y, *instance);
}

void Game::keyboardUpCallback(unsigned char key, int x, int y) {
    InputHandler::keyboardUp(key, x, y);
}

void Game::specialKeyDownCallback(int key, int x, int y) {
    InputHandler::specialKeyDown(key, x, y);
}

void Game::specialKeyUpCallback(int key, int x, int y) {
    InputHandler::specialKeyUp(key, x, y);
}

void Game::mouseCallback(int button, int state, int x, int y) {
    InputHandler::mouse(button, state, x, y, *instance);
}

void Game::updateCallback() {
    if (instance->getState() == GameState::PLAYING) {
        Player::update(*instance);
    }
}