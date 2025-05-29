#include "Maze.h"
#include "Renderer.h"
#include "Player.h"
#include <cmath>
#include <fstream>
#include <vector>
#include <cstring>

#define STB_IMAGE_IMPLEMENTATION
#include "C:\LabyrinthProject\include\stb_image.h"

// Структуры для WAD-формата
struct WADHeader {
    char magic[4]; // "IWAD" или "PWAD"
    int numLumps;
    int directoryOffset;
};

struct WADLump {
    int offset;
    int size;
    char name[8];
};

struct WADVertex {
    short x, y;
};

struct WADLineDef {
    short startVertex;
    short endVertex;
    short flags;
    short specialType;
    short sectorTag;
    short rightSideDef;
    short leftSideDef;
};

struct WADThing {
    short x, y;
    short angle;
    short type;
    short flags;
};

Maze::Maze() : width(20.0f), height(20.0f), exitX(0.0f), exitZ(0.0f), startX(0.0f), startZ(0.0f) {}

void Maze::loadFromImage(const std::string& filename) {
    int width, height, channels;
    unsigned char* image = stbi_load(filename.c_str(), &width, &height, &channels, 3);

    if (!image) {
        printf("Ошибка загрузки изображения: %s\n", filename.c_str());
        return;
    }

    walls.clear();

    float aspectRatio = (float)width / height;
    this->width = 20.0f;
    this->height = this->width / aspectRatio;
    float scaleX = this->width / width;
    float scaleZ = this->height / height;
    float wallThickness = 0.5f;

    // Установка начальной позиции игрока
    bool startFound = false;
    int startXPixel = 0;
    for (int x = 0; x < width && !startFound; x++) {
        int index = ((height - 1) * width + x) * 3; // Нижняя строка
        if (image[index] == 255 && image[index + 1] == 255 && image[index + 2] == 255) {
            startXPixel = x;
            startFound = true;
        }
    }
    if (startFound) {
        int shiftedX = startXPixel + 2;
        if (shiftedX >= width) shiftedX = width - 1;
        this->startX = (shiftedX * scaleX) - (this->width / 2) + (scaleX / 2);
        this->startZ = (this->height / 2) - (scaleZ / 2);
        Player::setX(this->startX);
        Player::setZ(this->startZ);
        Player::setAngle(M_PI);
    } else {
        this->startX = -this->width / 2 + 1.0f;
        this->startZ = this->height / 2 - 1.0f;
        Player::setX(this->startX);
        Player::setZ(this->startZ);
        Player::setAngle(M_PI);
    }

    // Установка позиции выхода
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
        exitX = (shiftedX * scaleX) - (this->width / 2) + (scaleX / 2);
        exitZ = -(this->height / 2) + (scaleZ / 2);
    } else {
        exitX = (this->width / 2) - 1.0f;
        exitZ = -(this->height / 2) + 1.0f;
    }

    // Генерация стен
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

                float x1 = (x * scaleX) - (this->width / 2);
                float z1 = (y * scaleZ) - (this->height / 2);
                float wallLength = wallWidth * scaleX;

                walls.push_back(x1);
                walls.push_back(z1);
                walls.push_back(wallLength);
                walls.push_back(wallThickness);
            }
        }
    }

    stbi_image_free(image);
}

void Maze::loadFromWAD(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        printf("Ошибка загрузки WAD: %s\n", filename.c_str());
        return;
    }

    walls.clear();

    // Чтение заголовка WAD
    WADHeader header;
    file.read((char*)&header, sizeof(WADHeader));
    if (strncmp(header.magic, "IWAD", 4) != 0 && strncmp(header.magic, "PWAD", 4) != 0) {
        printf("Неверный формат WAD-файла\n");
        file.close();
        return;
    }

    // Чтение директории
    file.seekg(header.directoryOffset);
    std::vector<WADLump> lumps(header.numLumps);
    for (int i = 0; i < header.numLumps; i++) {
        file.read((char*)&lumps[i], sizeof(WADLump));
    }

    // Поиск лампов карты
    int vertexesOffset = -1, vertexesSize = 0;
    int linedefsOffset = -1, linedefsSize = 0;
    int thingsOffset = -1, thingsSize = 0;
    for (const auto& lump : lumps) {
        char name[9] = {0};
        strncpy(name, lump.name, 8);
        if (strcmp(name, "VERTEXES") == 0) {
            vertexesOffset = lump.offset;
            vertexesSize = lump.size;
        } else if (strcmp(name, "LINEDEFS") == 0) {
            linedefsOffset = lump.offset;
            linedefsSize = lump.size;
        } else if (strcmp(name, "THINGS") == 0) {
            thingsOffset = lump.offset;
            thingsSize = lump.size;
        }
    }

    if (vertexesOffset == -1 || linedefsOffset == -1) {
        printf("Не найдены необходимые данные карты\n");
        file.close();
        return;
    }

    // Чтение вершин
    std::vector<WADVertex> vertices;
    file.seekg(vertexesOffset);
    int numVertices = vertexesSize / sizeof(WADVertex);
    vertices.resize(numVertices);
    file.read((char*)vertices.data(), vertexesSize);

    // Определение границ карты для масштабирования
    float minX = vertices[0].x, maxX = vertices[0].x;
    float minY = vertices[0].y, maxY = vertices[0].y;
    for (const auto& vertex : vertices) {
        minX = std::min(minX, (float)vertex.x);
        maxX = std::max(maxX, (float)vertex.x);
        minY = std::min(minY, (float)vertex.y);
        maxY = std::max(maxY, (float)vertex.y);
    }
    float mapWidth = maxX - minX;
    float mapHeight = maxY - minY;
    this->width = 20.0f;
    this->height = this->width * (mapHeight / mapWidth);
    float scaleX = this->width / mapWidth;
    float scaleZ = this->height / mapHeight;

    // Чтение линий (стен)
    file.seekg(linedefsOffset);
    int numLinedefs = linedefsSize / sizeof(WADLineDef);
    std::vector<WADLineDef> linedefs(numLinedefs);
    file.read((char*)linedefs.data(), linedefsSize);

    float wallThickness = 0.5f;
    int filteredWalls = 0;
    for (const auto& linedef : linedefs) {
        if (linedef.rightSideDef != -1) { // Линия с одной или двумя сторонами (стена)
            float x1 = vertices[linedef.startVertex].x;
            float z1 = vertices[linedef.startVertex].y;
            float x2 = vertices[linedef.endVertex].x;
            float z2 = vertices[linedef.endVertex].y;

            // Масштабирование координат
            x1 = (x1 - minX) * scaleX - (this->width / 2);
            z1 = (maxY - z1) * scaleZ - (this->height / 2);
            x2 = (x2 - minX) * scaleX - (this->width / 2);
            z2 = (maxY - z2) * scaleZ - (this->height / 2);

            float length = sqrt(pow(x2 - x1, 2) + pow(z2 - z1, 2));
            if (length < 0.1f) {
                filteredWalls++;
                continue;
            }

            // Фильтр стен за пределами карты
            if (fabs(x1) > this->width || fabs(x2) > this->width || fabs(z1) > this->height || fabs(z2) > this->height) {
                filteredWalls++;
                continue;
            }

            // Добавляем стену (ориентируем вдоль X или Z)
            if (fabs(x2 - x1) > fabs(z2 - z1)) {
                walls.push_back(std::min(x1, x2));
                walls.push_back(z1);
                walls.push_back(fabs(x2 - x1));
                walls.push_back(wallThickness);
            } else {
                walls.push_back(x1);
                walls.push_back(std::min(z1, z2));
                walls.push_back(wallThickness);
                walls.push_back(fabs(z2 - z1));
            }
        }
    }

    // Чтение объектов (начальная позиция и выход)
    bool startFound = false, exitFound = false;
    if (thingsOffset != -1) {
        file.seekg(thingsOffset);
        int numThings = thingsSize / sizeof(WADThing);
        std::vector<WADThing> things(numThings);
        file.read((char*)things.data(), thingsSize);

        for (const auto& thing : things) {
            if (thing.type == 1) { // Player 1 start
                this->startX = (thing.x - minX) * scaleX - (this->width / 2);
                this->startZ = (maxY - thing.y) * scaleZ - (this->height / 2);
                if (!findSafePlayerPosition(this->startX, this->startZ, false, 1.0f)) {
                    printf("Warning: Could not find safe player position near Thing, trying exhaustive search\n");
                    if (!findSafePlayerPosition(this->startX, this->startZ, true, 1.0f)) {
                        printf("Warning: Could not find safe player position, using default\n");
                        this->startX = -this->width / 2 + 1.0f;
                        this->startZ = this->height / 2 - 1.0f;
                    }
                }
                Player::setX(this->startX);
                Player::setZ(this->startZ);
                Player::setAngle(M_PI);
                startFound = true;
            } else if (thing.type == 11) { // Exit (пример)
                this->exitX = (thing.x - minX) * scaleX - (this->width / 2);
                this->exitZ = (maxY - thing.y) * scaleZ - (this->height / 2);
                exitFound = true;
            }
        }
    }

    // Установка позиций по умолчанию, если не найдены
    if (!startFound) {
        printf("Player start not found, trying exhaustive search\n");
        this->startX = -this->width / 2 + 1.0f;
        this->startZ = this->height / 2 - 1.0f;
        if (!findSafePlayerPosition(this->startX, this->startZ, true, 1.0f)) {
            printf("Warning: Could not find safe default player position\n");
        }
        Player::setX(this->startX);
        Player::setZ(this->startZ);
        Player::setAngle(M_PI);
    }
    if (!exitFound) {
        printf("Exit not found, using default\n");
        this->exitX = this->width / 2 - 1.0f;
        this->exitZ = -this->height / 2 + 1.0f;
    }

    // Проверка начальной позиции на коллизию
    for (size_t i = 0; i < walls.size(); i += 4) {
        if (Player::checkCollision(this->startX, this->startZ, walls[i], walls[i + 1], walls[i + 2], walls[i + 3])) {
            printf("Warning: Player start position is inside wall at x=%.2f, z=%.2f\n", walls[i], walls[i + 1]);
        }
    }

    file.close();
}

bool Maze::findSafePlayerPosition(float& x, float& z, bool exhaustiveSearch, float minClearRadius) {
    auto isPositionClear = [&](float testX, float testZ) {
        for (size_t i = 0; i < walls.size(); i += 4) {
            // Проверяем, чтобы точка не была в стене
            if (Player::checkCollision(testX, testZ, walls[i], walls[i + 1], walls[i + 2], walls[i + 3])) {
                return false;
            }
            // Проверяем, чтобы в радиусе minClearRadius не было стен
            float dx = std::max(0.0f, std::max(walls[i] - testX, testX - (walls[i] + walls[i + 2])));
            float dz = std::max(0.0f, std::max(walls[i + 1] - testZ, testZ - (walls[i + 1] + walls[i + 3])));
            float distance = sqrt(dx * dx + dz * dz);
            if (distance < minClearRadius) {
                return false;
            }
        }
        return true;
    };

    if (!exhaustiveSearch) {
        // Проверяем окрестности начальной позиции
        for (float offset = 0.5f; offset <= 2.0f; offset += 0.5f) {
            for (float dx = -offset; dx <= offset; dx += 0.5f) {
                for (float dz = -offset; dz <= offset; dz += 0.5f) {
                    float testX = x + dx;
                    float testZ = z + dz;
                    if (isPositionClear(testX, testZ)) {
                        x = testX;
                        z = testZ;
                        return true;
                    }
                }
            }
        }
        return false;
    } else {
        // Полный поиск по сетке
        float step = 0.5f;
        for (float testX = -this->width / 2; testX <= this->width / 2; testX += step) {
            for (float testZ = -this->height / 2; testZ <= this->height / 2; testZ += step) {
                if (isPositionClear(testX, testZ)) {
                    x = testX;
                    z = testZ;
                    return true;
                }
            }
        }
        return false;
    }
}

void Maze::resetPlayerPosition() {
    Player::setX(this->startX);
    Player::setZ(this->startZ);
    Player::setAngle(M_PI);
}