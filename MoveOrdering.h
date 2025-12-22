#pragma once
#include "Board.h"
#include "MoveList.h"

class MoveOrdering {
public:
    static void SortMoves(
        const Board& board,
        MoveList& moves,
        Move ttMove,
        Move killer1,
        Move killer2,
        const int history[2][64][64]
    );
};