#include <iostream>
#include "BoardManager.h"

int main() {
    bool isWhiteRobot = false;
    bool isBlackRobot = true;
    BoardManager bm(isWhiteRobot, isBlackRobot);
    bm.board.PrintBoard();

    bm.startGameLoop();

    return 0;
}