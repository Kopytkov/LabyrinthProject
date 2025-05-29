#ifndef MAZE_H
#define MAZE_H

#include <vector>
#include <string>

// Задание: создать класс Loader. От него 2 функции для PNG и WAD файлов

class Maze {
public:
    Maze();
    void loadFromImage(const std::string& filename);
    void loadFromWAD(const std::string& filename);
    void resetPlayerPosition();
    bool findSafePlayerPosition(float& x, float& z, bool exhaustiveSearch = false, float minClearRadius = 1.0f); // Добавлен minClearRadius

    float getWidth() const { return width; }
    float getHeight() const { return height; }
    float getExitX() const { return exitX; }
    float getExitZ() const { return exitZ; }
    const std::vector<float>& getWalls() const { return walls; }

    static Maze& getInstance() {
        static Maze instance;
        return instance;
    }

private:
    float width;
    float height;
    float exitX, exitZ;
    float startX, startZ;
    std::vector<float> walls;
};

#endif