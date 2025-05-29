#include "Player.h"
#include "Maze.h"
#include "InputHandler.h"
#include <cmath>

float Player::x = 0.0f;
float Player::y = 0.0f;
float Player::z = 0.0f;
float Player::angle = 0.0f;

void Player::update(Game& game) {
    float speed = 0.005f;
    float rotSpeed = 0.005f;
    float newX = x, newZ = z;

    if (InputHandler::isSpecialKeyPressed(GLUT_KEY_UP) || InputHandler::isKeyPressed('w') || InputHandler::isKeyPressed('W')) {
        newZ += speed * cos(angle);
        newX += speed * sin(angle);
    }
    if (InputHandler::isSpecialKeyPressed(GLUT_KEY_DOWN) || InputHandler::isKeyPressed('s') || InputHandler::isKeyPressed('S')) {
        newZ -= speed * cos(angle);
        newX -= speed * sin(angle);
    }
    if (InputHandler::isSpecialKeyPressed(GLUT_KEY_LEFT) || InputHandler::isKeyPressed('a') || InputHandler::isKeyPressed('A')) {
        newX += speed * cos(angle);
        newZ -= speed * sin(angle);
    }
    if (InputHandler::isSpecialKeyPressed(GLUT_KEY_RIGHT) || InputHandler::isKeyPressed('d') || InputHandler::isKeyPressed('D')) {
        newX -= speed * cos(angle);
        newZ += speed * sin(angle);
    }

    if (InputHandler::isKeyPressed('q') || InputHandler::isKeyPressed('Q')) {
        angle += rotSpeed;
    }
    if (InputHandler::isKeyPressed('e') || InputHandler::isKeyPressed('E')) {
        angle -= rotSpeed;
    }

    bool collision = false;
    const std::vector<float>& walls = Maze::getInstance().getWalls();
    for (size_t i = 0; i < walls.size(); i += 4) {
        if (checkCollision(newX, newZ, walls[i], walls[i + 1], walls[i + 2], walls[i + 3])) {
            collision = true;
            break;
        }
    }

    if (!collision) {
        x = newX;
        z = newZ;
    }

    if (fabs(x - Maze::getInstance().getExitX()) < 0.5f && fabs(z - Maze::getInstance().getExitZ()) < 0.5f) {
        game.setState(GameState::WIN);
    }

    glutPostRedisplay();
}

bool Player::checkCollision(float newX, float newZ, float x, float z, float width, float height) {
    float minX = x - 0.2f;
    float maxX = x + width + 0.2f;
    float minZ = z - 0.2f;
    float maxZ = z + height + 0.2f;
    return (newX >= minX && newX <= maxX && newZ >= minZ && newZ <= maxZ);
}