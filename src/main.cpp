#include "Game.h"

int main(int argc, char** argv) {
    Game game;
    game.initialize(argc, argv);
    game.run();
    return 0;
}