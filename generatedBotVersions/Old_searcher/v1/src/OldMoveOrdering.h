#pragma once
#include "Board.h"
#include "MoveList.h"

namespace OldMoveOrdering {

    class MoveOrdering {
    public:
        static int See(const Board& board, Move m);
        static void SortMoves(
            const Board& board,
            MoveList& moves
        );
    };
}
