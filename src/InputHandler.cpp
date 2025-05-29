#include "InputHandler.h"
#include "Maze.h"
#include "Renderer.h"
#include "Player.h"
#include <cmath>
#include <algorithm> // Для std::string::find

bool InputHandler::keys[256] = {false};
bool InputHandler::specialKeys[256] = {false};

void InputHandler::keyboard(unsigned char key, int x, int y, Game& game) {
    keys[key] = true;
    if (game.getState() == GameState::PLAYING) {
        if (key == 'm' || key == 'M') {
            game.toggleMiniMap();
            glutPostRedisplay();
        }
    } else if (game.getState() == GameState::WIN && game.getActiveMessage() != -1) {
        if (key == 'y' || key == 'Y') {
            if (game.getActiveMessage() == 0) {  // Start again
                std::string level = game.getCurrentLevel();
                Maze::getInstance().loadFromImage("../LabyrinthProject/" + level);
                if (!Maze::getInstance().getWalls().empty()) {
                    Renderer::loadTexture("../LabyrinthProject/wall_texture.png", Renderer::wallTexture);
                    Renderer::loadTexture("../LabyrinthProject/floor_texture.png", Renderer::floorTexture);
                    game.setActiveMessage(-1);
                    game.setMiniMapShown(false);
                    Maze::getInstance().resetPlayerPosition();
                    game.setState(GameState::PLAYING);
                    glutPostRedisplay();
                }
            } else if (game.getActiveMessage() == 1) {  // Exit
                exit(0);
            } else if (game.getActiveMessage() == 2) {  // Go back to the menu
                game.setState(GameState::MENU);
                game.setActiveMessage(-1);
                glutDisplayFunc(Game::displayCallback);
                glutPostRedisplay();
            }
        } else if (key == 'n' || key == 'N') {
            game.setActiveMessage(-1);
            glutPostRedisplay();
        }
    }
}

void InputHandler::keyboardUp(unsigned char key, int x, int y) {
    keys[key] = false;
}

void InputHandler::specialKeyDown(int key, int x, int y) {
    specialKeys[key] = true;
}

void InputHandler::specialKeyUp(int key, int x, int y) {
    specialKeys[key] = false;
}

void InputHandler::mouse(int button, int state, int x, int y, Game& game) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        int windowWidth = game.getWindowWidth();
        int windowHeight = game.getWindowHeight();
        y = windowHeight - y;

        if (game.getState() == GameState::MENU) {
            float menuButtonWidth = 0.25f * windowWidth;
            float menuButtonHeight = 0.083f * windowHeight;
            float menuXStart = 0.375f * windowWidth;
            float menuXEnd = menuXStart + menuButtonWidth;

            if (x >= menuXStart && x <= menuXEnd && y >= 0.583f * windowHeight && y <= 0.666f * windowHeight) {  // Easy
                Maze::getInstance().loadFromImage("../LabyrinthProject/maze_easy.png");
                if (!Maze::getInstance().getWalls().empty()) {
                    Renderer::loadTexture("../LabyrinthProject/wall_texture.png", Renderer::wallTexture);
                    Renderer::loadTexture("../LabyrinthProject/floor_texture.png", Renderer::floorTexture);
                    game.setState(GameState::PLAYING);
                    game.setCurrentLevel("maze_easy.png");
                    game.setMiniMapShown(false);
                    glutPostRedisplay();
                }
            } else if (x >= menuXStart && x <= menuXEnd && y >= 0.458f * windowHeight && y <= 0.541f * windowHeight) {  // Medium
                Maze::getInstance().loadFromImage("../LabyrinthProject/maze_medium.png");
                if (!Maze::getInstance().getWalls().empty()) {
                    Renderer::loadTexture("../LabyrinthProject/wall_texture.png", Renderer::wallTexture);
                    Renderer::loadTexture("../LabyrinthProject/floor_texture.png", Renderer::floorTexture);
                    game.setState(GameState::PLAYING);
                    game.setCurrentLevel("maze_medium.png");
                    game.setMiniMapShown(false);
                    glutPostRedisplay();
                }
            } else if (x >= menuXStart && x <= menuXEnd && y >= 0.333f * windowHeight && y <= 0.416f * windowHeight) {  // Hard
                Maze::getInstance().loadFromImage("../LabyrinthProject/maze_hard.png");
                if (!Maze::getInstance().getWalls().empty()) {
                    Renderer::loadTexture("../LabyrinthProject/wall_texture.png", Renderer::wallTexture);
                    Renderer::loadTexture("../LabyrinthProject/floor_texture.png", Renderer::floorTexture);
                    game.setState(GameState::PLAYING);
                    game.setCurrentLevel("maze_hard.wad");
                    game.setMiniMapShown(false);
                    glutPostRedisplay();
                }
            }
        } else if (game.getState() == GameState::WIN) {
            float msgWidth = 0.25f * windowWidth;
            float msgHeight = 0.033f * windowHeight;
            float msgXStart = 0.375f * windowWidth;
            float msgXEnd = msgXStart + msgWidth;
            float btnWidth = 0.0625f * windowWidth;
            float btnHeight = 0.033f * windowHeight;

            if (x >= msgXStart && x <= msgXEnd && y >= 0.566f * windowHeight && y <= 0.6f * windowHeight) {  // Start again?
                game.setActiveMessage(0);
                glutPostRedisplay();
            } else if (x >= msgXStart && x <= msgXEnd && y >= 0.483f * windowHeight && y <= 0.516f * windowHeight) {  // Exit
                game.setActiveMessage(1);
                glutPostRedisplay();
            } else if (x >= msgXStart && x <= msgXEnd && y >= 0.4f * windowHeight && y <= 0.433f * windowHeight) {  // Go back
                game.setActiveMessage(2);
                glutPostRedisplay();
            } else if (game.getActiveMessage() == 0) {
                if (x >= 0.4375f * windowWidth && x <= 0.5f * windowWidth && y >= 0.533f * windowHeight && y <= 0.566f * windowHeight) {  // YES для Start again
                    std::string level = game.getCurrentLevel();
                    if (level.find(".wad") != std::string::npos) {
                        Maze::getInstance().loadFromWAD("../LabyrinthProject/" + level);
                    } else {
                        Maze::getInstance().loadFromImage("../LabyrinthProject/" + level);
                    }
                    if (!Maze::getInstance().getWalls().empty()) {
                        Renderer::loadTexture("../LabyrinthProject/wall_texture.png", Renderer::wallTexture);
                        Renderer::loadTexture("../LabyrinthProject/floor_texture.png", Renderer::floorTexture);
                        game.setActiveMessage(-1);
                        game.setMiniMapShown(false);
                        Maze::getInstance().resetPlayerPosition();
                        game.setState(GameState::PLAYING);
                        glutPostRedisplay();
                    }
                } else if (x >= 0.5125f * windowWidth && x <= 0.575f * windowWidth && y >= 0.533f * windowHeight && y <= 0.566f * windowHeight) {  // NO
                    game.setActiveMessage(-1);
                    glutPostRedisplay();
                }
            } else if (game.getActiveMessage() == 1) {
                if (x >= 0.4375f * windowWidth && x <= 0.5f * windowWidth && y >= 0.45f * windowHeight && y <= 0.483f * windowHeight) {  // YES для Exit
                    exit(0);
                } else if (x >= 0.5125f * windowWidth && x <= 0.575f * windowWidth && y >= 0.45f * windowHeight && y <= 0.483f * windowHeight) {  // NO
                    game.setActiveMessage(-1);
                    glutPostRedisplay();
                }
            } else if (game.getActiveMessage() == 2) {
                if (x >= 0.4375f * windowWidth && x <= 0.5f * windowWidth && y >= 0.366f * windowHeight && y <= 0.4f * windowHeight) {  // YES для Go back
                    game.setState(GameState::MENU);
                    game.setActiveMessage(-1);
                    glutPostRedisplay();
                } else if (x >= 0.5125f * windowWidth && x <= 0.575f * windowWidth && y >= 0.366f * windowHeight && y <= 0.4f * windowHeight) {  // NO
                    game.setActiveMessage(-1);
                    glutPostRedisplay();
                }
            }
        }
    }
}