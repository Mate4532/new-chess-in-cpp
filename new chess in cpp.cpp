#include <iostream>
#include "BoardManager.h"

int main() {
    BoardManager bm(false, true);
    bm.board.PrintBoard();

    bm.startGameLoop();

    return 0;
}