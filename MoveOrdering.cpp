#include "MoveOrdering.h"
#include "Evaluation.h"

static inline int ScoreMove(
    const Board& board,
    const Move& m,
    const Move& ttMove,
    const Move killerMoves[MAX_PLY][2],
    int ply,
    const int history[2][MAX_KILLER_HISTORY][MAX_KILLER_HISTORY]
) {
    Color us = board.getSideToMove();
    Color enemy = (Color)(us ^ 1);

    if (m.getFrom() == ttMove.getFrom() &&
        m.getTo() == ttMove.getTo() &&
        m.getFlags() == ttMove.getFlags())
        return 10'000'000;

    if (m.getFlags() & CAPTURE_FLAG) {
        PieceType victim =
            (m.getFlags() == EN_PASSANT)
            ? PAWN
            : board.getPieceAt(m.getTo(), enemy);

        int v = Evaluation::GetPieceValue(victim);
        int a = Evaluation::GetPieceValue(m.getPieceType());
        return 5'000'000 + v * 16 - a;
    }

    if (m.getMoveData() == killerMoves[ply][0].getMoveData()) return 900'000;
    if (m.getMoveData() == killerMoves[ply][1].getMoveData()) return 800'000;

    if (m.getFlags() & PROMOTION_FLAG) {
        if ((m.getFlags() & 0b0011) == PROMOTION_TYPE_QUEEN)
            return 9'000'000;
        return 3'000'000;
    }

    return history[us][m.getFrom()][m.getTo()];
}

void MoveOrdering::SortMoves(
    const Board& board,
    MoveList& moves,
    Move ttMove,
    const Move killerMoves[MAX_PLY][2],
    int ply,
    const int history[2][MAX_KILLER_HISTORY][MAX_KILLER_HISTORY]
) {
    int scores[256];

    int n = (int)moves.size();
    for (int i = 0; i < n; i++) {
        scores[i] = ScoreMove(
            board,
            moves[i],
            ttMove,
            killerMoves,
            ply,
            history
        );
    }

    for (int i = 0; i < n - 1; i++) {
        int best = i;
        for (int j = i + 1; j < n; j++) {
            if (scores[j] > scores[best])
                best = j;
        }
        if (best != i) {
            std::swap(scores[i], scores[best]);
            std::swap(moves[i], moves[best]);
        }
    }
}

void MoveOrdering::SortQuiescenceMoves(const Board& board, MoveList& moves) {
    int scores[256];
    int n = (int)moves.size();

    for (int i = 0; i < n; i++) {
        const Move& m = moves[i];

        PieceType victim = board.getPieceAt(m.getTo(), (Color)(board.getSideToMove() ^ 1));
        PieceType attacker = m.getPieceType();

        scores[i] = Evaluation::GetPieceValue(victim) * 10 - Evaluation::GetPieceValue(attacker);

        if (m.getFlags() & PROMOTION_FLAG) scores[i] += 10000;
    }

    for (int i = 0; i < n - 1; i++) {
        int best = i;
        for (int j = i + 1; j < n; j++) {
            if (scores[j] > scores[best]) best = j;
        }
        std::swap(scores[i], scores[best]);
        std::swap(moves[i], moves[best]);
    }
}
