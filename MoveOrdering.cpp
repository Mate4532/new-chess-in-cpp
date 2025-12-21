#include "MoveOrdering.h"

void MoveOrdering::SortMoves(const Board& board, MoveList& moves, Move ttMove,
    Move killer1, Move killer2, const int history[2][64][64]) {
    if (moves.size() <= 1) return;

    int scores[256] = { 0 };
    Color us = board.getSideToMove();
    Color enemy = (Color)(us ^ 1);

    for (int i = 0; i < (int)moves.size(); i++) {
        const Move& m = moves[i];
        int score = 0;

        if (m.getFrom() == ttMove.getFrom() && m.getTo() == ttMove.getTo() && m.getFlags() == ttMove.getFlags()) {
            score = 1000000;
        }
        else if (m.getFlags() & CAPTURE_FLAG) {
            PieceType attacker = m.getPieceType();
            PieceType victim = board.getPieceAt(m.getTo(), enemy);
            if (m.getFlags() == EN_PASSANT) victim = PAWN;

            score = 900000 + (Evaluation::GetPieceValue(victim) * 10) - Evaluation::GetPieceValue(attacker);
        }

        else if (m.getFlags() & PROMOTION_FLAG) {
            MoveFlag prom = (MoveFlag)(m.getFlags() & 0b0011);
            if (prom == PROMOTION_TYPE_QUEEN) score = 850000;
            else score = 300000;
        }

        else if (m.getFrom() == killer1.getFrom() && m.getTo() == killer1.getTo()) {
            score = 800000;
        }
        else if (m.getFrom() == killer2.getFrom() && m.getTo() == killer2.getTo()) {
            score = 700000;
        }

        else {
            score = history[us][m.getFrom()][m.getTo()];
        }

        scores[i] = score;
    }

    for (int i = 0; i < (int)moves.size() - 1; i++) {
        int bestIdx = i;
        for (int j = i + 1; j < (int)moves.size(); j++) {
            if (scores[j] > scores[bestIdx]) bestIdx = j;
        }
        std::swap(scores[i], scores[bestIdx]);
        std::swap(moves[i], moves[bestIdx]);
    }
}