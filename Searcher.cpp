#include "Searcher.h"
#include "Evaluation.h"
#include <chrono>
#include <iostream>

inline long long now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

inline int Searcher::ScoreToTT(int score, int ply) {
    if (IsMateScore(score)) {
        return score > 0 ? score + ply : score - ply;
    }
    return score;
}

inline int Searcher::ScoreFromTT(int score, int ply) {
    if (IsMateScore(score)) {
        return score > 0 ? score - ply : score + ply;
    }
    return score;
}

int Searcher::quiescence(int alpha, int beta) {
    nodes++;
    if ((nodes & 2047) == 0 && now_ms() - startTime >= robot_thinking_time_ms)
        stop = true;
    if (stop) return alpha;

    int standPat = Evaluation::EvaluatePos(board);
    if (standPat >= beta) return beta;
    if (standPat > alpha) alpha = standPat;

    MoveList moves;
    MoveGenerator::GenerateMoves(board, moves, true);

    MoveOrdering::SortMoves(board, moves, Move(), historyMoves);

    for (const Move& m : moves) {
        if (!board.MakeMove(m)) continue;
        int score = -quiescence(-beta, -alpha);
        board.UndoMove(m);

        if (stop) return alpha;
        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

int Searcher::negamax(int depth, int alpha, int beta, int ply) {

    nodes++;
    if ((nodes & 2047) == 0 && now_ms() - startTime >= robot_thinking_time_ms)
        stop = true;
    if (stop)
        return alpha;

    int originalAlpha = alpha;
    uint64_t hash = board.getHash();

    int ttScore;
    Move ttMove;

    bool inCheckBeforeMove = board.isSquareAttacked(
        board.getKingSquare(board.getSideToMove()),
        (Color)(board.getSideToMove() ^ 1));

    if (inCheckBeforeMove)
        depth++;

    if (tt.Probe(hash, depth, alpha, beta, ttScore, ttMove)) {
        return ScoreFromTT(ttScore, ply);
    }

    if (depth <= 0)
        return quiescence(alpha, beta);

    if (depth >= 4 && !inCheckBeforeMove && ply > 0) {

        Color us = board.getSideToMove();

        bool hasBigPiece =
            board.getPieceBitboard(us, QUEEN) ||
            board.getPieceBitboard(us, ROOK);

        bool hasMinor =
            board.getPieceBitboard(us, BISHOP) ||
            board.getPieceBitboard(us, KNIGHT);

        bool zugzwangRisk =
            !hasBigPiece &&
            board.getPieceBitboard(us, PAWN);

        if ((hasBigPiece || hasMinor) && !zugzwangRisk) {

            board.MakeNullMove();
            int r = 2 + depth / 6;
            int score = -negamax(depth - 1 - r, -beta, -beta + 1, ply + 1);
            board.UndoNullMove();

            if (score >= beta)
                return beta;
        }
    }

    MoveList moves;
    MoveGenerator::GenerateMoves(board, moves);

    if (ply < MAX_KILLER_HISTORY) {

        MoveOrdering::SortMoves(
            board,
            moves,
            ttMove,
            historyMoves
        );
    }

    Move bestMove;
    int legalMoves = 0;

    for (const Move& m : moves) {
        if (!board.MakeMove(m))
            continue;

        legalMoves++;

        int score;
        bool quiet =
            !(m.getFlags() & CAPTURE_FLAG) &&
            !(m.getFlags() & PROMOTION_FLAG);

        bool inCheckAfterMove = board.isSquareAttacked(
            board.getKingSquare(board.getSideToMove()),
            (Color)(board.getSideToMove() ^ 1));

        if (depth >= 3 && legalMoves > 3 && quiet && !inCheckAfterMove) {
            score = -negamax(depth - 2, -alpha - 1, -alpha, ply + 1);
            if (score > alpha)
                score = -negamax(depth - 1, -beta, -alpha, ply + 1);
        }
        else {
            score = -negamax(depth - 1, -beta, -alpha, ply + 1);
        }

        board.UndoMove(m);
        if (stop)
            return alpha;

        if (score >= beta && ply < MAX_KILLER_HISTORY) {
            if (quiet) {
                historyMoves[board.getSideToMove()][m.getFrom()][m.getTo()] += depth * depth;
            }
            tt.Store(hash, ScoreToTT(score, ply), depth, TT_BETA, m);
            return score;
        }

        if (score > alpha) {
            alpha = score;
            bestMove = m;
        }
    }

    if (legalMoves == 0) {
        int score = inCheckBeforeMove ? -MATE_SCORE + ply : 0;

        return score;
    }

    TTFlag flag = (alpha <= originalAlpha) ? TT_ALPHA : TT_EXACT;
    tt.Store(hash, ScoreToTT(alpha, ply), depth, flag, bestMove);

    return alpha;
}

void Searcher::ClearHistory() {
    for (int c = 0; c < 2; c++)
        for (int f = 0; f < 64; f++)
            for (int t = 0; t < 64; t++)
                historyMoves[c][f][t] = 0;
}

void Searcher::AgeHistory() {
    for (int c = 0; c < 2; c++)
        for (int f = 0; f < 64; f++)
            for (int t = 0; t < 64; t++)
                historyMoves[c][f][t] >>= 1;
}

Move Searcher::IterativeDeepening() {
    startTime = now_ms();
    stop = false;
    nodes = 0;
    tt.NewWrite();
    AgeHistory();

    Move bestMove;
    int lastScore = 0;

    for (int depth = 1; depth <= max_depth; depth++) {
        int window = 50;
        int alpha = lastScore - window;
        int beta = lastScore + window;
        int score;

        while (true) {
            score = negamax(depth, alpha, beta, 0);
            if (stop) break;
            if (score <= alpha) {
                alpha -= window;
            }
            else if (score >= beta) {
                beta += window;
            }
            else {
                break;
            }
            window *= 2;
        }

        int rawScore;
        Move tmpMove;
        if (tt.Probe(board.getHash(), depth, -MATE_SCORE, MATE_SCORE, rawScore, tmpMove)) {
            score = ScoreFromTT(rawScore, 0);
        }

        if (tmpMove.isValid()) {
            bestMove = tmpMove;
        }
        
        if (stop) break;

        lastScore = score;

        std::cout << "info depth " << depth << " score ";
        if (abs(score) > 90000)
            std::cout << "mate " << ((score > 0) ? (100001 - score) / 2 : -(100001 + score) / 2);
        else
            std::cout << "cp " << (board.getSideToMove() == WHITE ? score : -score);

        std::cout << " time " << (now_ms() - startTime)
            << " nodes " << nodes
            << " pv " << bestMove.toAlgebraic()
            << std::endl;

        if (abs(score) > 90000)
            break;
    }

    std::cout << "Bestmove: " << bestMove.toAlgebraic()
        << " score cp "
        << (board.getSideToMove() == WHITE ? lastScore : -lastScore)
        << std::endl;

    return bestMove;
}

Move Searcher::GetBestMove() {
    return IterativeDeepening();
}
