#ifndef GAME_H
#define GAME_H

#include <GL/freeglut.h>
#include <string>

enum class GameState { MENU, PLAYING, WIN };

class Game {
public:
    Game();
    void initialize(int argc, char** argv);
    void run();

    GameState getState() const { return state; }
    void setState(GameState newState) { state = newState; }
    bool isMiniMapShown() const { return showMiniMap; }
    void toggleMiniMap() { showMiniMap = !showMiniMap; }
    void setMiniMapShown(bool shown) { showMiniMap = shown; }
    int getActiveMessage() const { return activeMessage; }
    void setActiveMessage(int msg) { activeMessage = msg; }
    std::string getCurrentLevel() const { return currentLevel; }
    void setCurrentLevel(const std::string& level) { currentLevel = level; }
    int getWindowWidth() const { return windowWidth; }
    int getWindowHeight() const { return windowHeight; }

    static Game* instance;
    static void displayCallback();

private:
    GameState state;
    bool showMiniMap;
    int activeMessage;
    std::string currentLevel;
    int windowWidth;
    int windowHeight;

    static void reshapeCallback(int w, int h);
    static void keyboardCallback(unsigned char key, int x, int y);
    static void keyboardUpCallback(unsigned char key, int x, int y);
    static void specialKeyDownCallback(int key, int x, int y);
    static void specialKeyUpCallback(int key, int x, int y);
    static void mouseCallback(int button, int state, int x, int y);
    static void updateCallback();
};

#endif