#pragma once
#include "Board.h"
#include "MoveList.h"

#define MAX_KILLER_HISTORY 128

class MoveOrdering {
public:
    static void SortMoves(
        const Board& board,
        MoveList& moves,
        Move ttMove,
        const int history[2][MAX_KILLER_HISTORY][MAX_KILLER_HISTORY]
    );
};