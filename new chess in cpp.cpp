#include <iostream>
#include "BoardManager.h"

int main() {
    bool isWhiteRobot = true;
    bool isBlackRobot = false;
    BoardManager bm(isWhiteRobot, isBlackRobot);
    bm.board.PrintBoard();

    bm.startGameLoop();

    return 0;
}