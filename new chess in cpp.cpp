#include <iostream>
#include "BoardManager.h"

int main() {
    BoardManager bm(true, false);
    bm.board.PrintBoard();

    bm.startGameLoop();

    return 0;
}