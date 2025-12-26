#pragma once
#include "Board.h"
#include "MoveList.h"

#define MAX_KILLER_HISTORY 64

class MoveOrdering {
public:
    static void SortMoves(
        const Board& board,
        MoveList& moves,
        Move ttMove,
        const Move killerMoves[MAX_PLY][2],
        int ply,
        const int history[2][MAX_KILLER_HISTORY][MAX_KILLER_HISTORY]
    );
    static void SortQuiescenceMoves(const Board& board, MoveList& moves);
};