#include "MoveOrdering.h"
#include "Evaluation.h"

static inline int ScoreMove(
    const Board& board,
    const Move& m,
    const Move& ttMove,
    const int history[2][MAX_KILLER_HISTORY][MAX_KILLER_HISTORY]
) {

    if (m.getMoveData() == ttMove.getMoveData()) return 10'000'000;

    if (m.getFlags() & CAPTURE_FLAG) {
        Color enemy = (Color)(board.getSideToMove() ^ 1);
        PieceType victim = (m.getFlags() & EN_PASSANT) ? PAWN : board.getPieceAt(m.getTo(), enemy);
        int v = Evaluation::GetPieceValue(victim);
        int a = Evaluation::GetPieceValue(m.getPieceType());
        return 5'000'000 + v * 16 - a;
    }

    if (m.getFlags() & PROMOTION_FLAG) {
        if ((m.getFlags() & 0b0011) == PROMOTION_TYPE_QUEEN) return 4'000'000;
        return 3'000'000;
    }

    return history[board.getSideToMove()][m.getFrom()][m.getTo()];
}

void MoveOrdering::SortMoves(
    const Board& board,
    MoveList& moves,
    Move ttMove,
    const int history[2][MAX_KILLER_HISTORY][MAX_KILLER_HISTORY]
) {
    int scores[256];
    int n = (int)moves.size();
    for (int i = 0; i < n; i++) {
        scores[i] = ScoreMove(board, moves[i], ttMove, history);
    }

    for (int i = 0; i < n - 1; i++) {
        int best = i;
        for (int j = i + 1; j < n; j++) {
            if (scores[j] > scores[best]) best = j;
        }
        if (best != i) {
            std::swap(scores[i], scores[best]);
            std::swap(moves[i], moves[best]);
        }
    }
}