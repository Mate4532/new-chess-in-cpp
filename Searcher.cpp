#include "Searcher.h"
#include "Evaluation.h"
#include <chrono>
#include <iostream>

inline long long now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

inline int ScoreToTT(int score, int ply) {
    if (score > 90000) return score + ply;
    if (score < -90000) return score - ply;
    return score;
}

inline int ScoreFromTT(int score, int ply) {
    if (score > 90000) return score - ply;
    if (score < -90000) return score + ply;
    return score;
}

int Searcher::quiescence(int alpha, int beta) {
    static int nodes_visited = 0;
    if ((nodes_visited++ & 2047) == 0) {
        if (now_ms() - startTime >= robot_thinking_time_ms) stopSearch = true;
    }
    if (stopSearch) return alpha;

    int standing_pat = Evaluation::EvaluatePos(m_board);
    if (standing_pat >= beta) return beta;
    if (alpha < standing_pat) alpha = standing_pat;

    MoveList moves;
    MoveGenerator::GenerateMoves(m_board, moves, true);

    MoveOrdering::SortMoves(
        m_board,
        moves,
        Move(),
        Move(),
        Move(),
        historyMoves
    );

    for (const auto& move : moves) {
        if (!m_board.MakeMove(move)) continue;

        int score = -quiescence(-beta, -alpha);
        m_board.UndoMove(move);

        if (stopSearch) return alpha;
        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

int Searcher::negamax(int depth, int alpha, int beta, int ply) {

    if (ply >= 64) return Evaluation::EvaluatePos(m_board);

    uint64_t hash = m_board.getHash();
    int ttScore;
    Move ttMove = Move();
    int originalAlpha = alpha;

    if (tt.Probe(hash, depth, alpha, beta, ttScore, ttMove)) {
        return ScoreFromTT(ttScore, ply);
    }

    if (ply > 0 && m_board.IsRepetition()) {
        return 0;
    }

    static int nodes_visited = 0;
    if ((nodes_visited++ & 2047) == 0) {
        if (now_ms() - startTime >= robot_thinking_time_ms) stopSearch = true;
    }
    if (stopSearch) return alpha;

    Color us = m_board.getSideToMove();
    bool inCheck = m_board.isSquareAttacked(m_board.getKingSquare(us), (Color)(us ^ 1));

    if (inCheck && ply < 64) depth++;

    if (depth <= 0) return quiescence(alpha, beta);

    if (depth >= 3 && !inCheck && ply > 0) {
        uint64_t nonPawns = m_board.getSideOccupancy(us) ^ m_board.getPieceBitboard(us, PAWN) ^ m_board.getPieceBitboard(us, KING);
        if (nonPawns != 0) {
            m_board.MakeNullMove();
            int reduction = 2 + (depth / 6);
            int score = -negamax(depth - 1 - reduction, -beta, -beta + 1, ply + 1);
            m_board.UndoNullMove();
            if (score >= beta) return beta;
        }
    }

    MoveList moves;
    MoveGenerator::GenerateMoves(m_board, moves);
    MoveOrdering::SortMoves(
        m_board,
        moves,
        ttMove,
        killerMoves[0][ply],
        killerMoves[1][ply],
        historyMoves
    );

    int legal_moves_count = 0;
    Move best_move_this_node = Move();

    for (int i = 0; i < moves.size(); i++) {
        Move move = moves[i];
        if (!m_board.MakeMove(move)) continue;
        legal_moves_count++;

        int score;
        bool isCapture = (move.getFlags() & CAPTURE_FLAG);
        bool isPromotion = (move.getFlags() & PROMOTION_FLAG);

        if (depth >= 3 && legal_moves_count > 3 && !isCapture && !isPromotion && !inCheck) {
            int reduction = (depth > 6) ? 2 : 1;
            score = -negamax(depth - 1 - reduction, -alpha - 1, -alpha, ply + 1);
            if (score > alpha) score = -negamax(depth - 1, -beta, -alpha, ply + 1);
        }
        else {
            score = -negamax(depth - 1, -beta, -alpha, ply + 1);
        }

        m_board.UndoMove(move);
        if (stopSearch) return alpha;

        if (score >= beta) {
            if (!isCapture && ply < 64) {
                if (!(move.getFrom() == killerMoves[0][ply].getFrom() && move.getTo() == killerMoves[0][ply].getTo())) {
                    killerMoves[1][ply] = killerMoves[0][ply];
                    killerMoves[0][ply] = move;
                }
                historyMoves[us][move.getFrom()][move.getTo()] += depth * depth;
            }
            tt.Store(hash, ScoreToTT(beta, ply), depth, BETA, move);
            return beta;
        }
        if (score > alpha) {
            alpha = score;
            best_move_this_node = move;
        }
    }

    if (legal_moves_count == 0) {
        if (stopSearch) return alpha;
        if (inCheck) return -100000 + ply;
        return 0;
    }

    TTFlag flag = (alpha <= originalAlpha) ? ALPHA : EXACT;
    tt.Store(hash, ScoreToTT(alpha, ply), depth, flag, best_move_this_node);

    return alpha;
}

void Searcher::ClearHistory() {
    for (int color = 0; color < 2; color++) {
        for (int from = 0; from < 64; from++) {
            for (int to = 0; to < 64; to++) {
                historyMoves[color][from][to] = 0;
            }
        }
    }
}

void Searcher::AgeHistory() {
    for (int color = 0; color < 2; color++) {
        for (int from = 0; from < 64; from++) {
            for (int to = 0; to < 64; to++) {
                historyMoves[color][from][to] >>= 1;
            }
        }
    }
}

Move Searcher::IterativeDeepening() {
    startTime = now_ms();
    stopSearch = false;
    AgeHistory();

    m_board.getRepetitionTable().Init(m_board);

    Move best_so_far = Move();
    int last_score = 0;

    for (int current_depth = 1; current_depth <= max_depth; ++current_depth) {
        int alpha = -1000000;
        int beta = 1000000;

        MoveList moves;
        MoveGenerator::GenerateMoves(m_board, moves);
        MoveOrdering::SortMoves(
            m_board,
            moves,
            best_so_far,
            killerMoves[0][0],
            killerMoves[1][0],
            historyMoves
        );

        int best_score_this_depth = -1000000;
        Move best_move_this_depth = Move();

        for (const auto& move : moves) {
            if (!m_board.MakeMove(move)) continue;
            int score = -negamax(current_depth - 1, -beta, -alpha, 1);
            m_board.UndoMove(move);

            if (stopSearch) break;

            if (score > best_score_this_depth) {
                best_score_this_depth = score;
                best_move_this_depth = move;
            }
            if (score > alpha) alpha = score;
        }

        if (stopSearch) break;

        if (best_move_this_depth.getPieceType() != PIECE_NONE) {
            best_so_far = best_move_this_depth;
            last_score = best_score_this_depth;
            tt.Store(m_board.getHash(), ScoreToTT(last_score, 0), current_depth, EXACT, best_so_far);

            std::cout << "info depth " << current_depth << ", ";

            if (last_score > 90000)
                std::cout << "score mate " << (100001 - last_score) / 2 + 1;
            else if (last_score < -90000)
                std::cout << "score mate -" << (100001 + last_score) / 2 + 1;
            else
                std::cout << "score cp " << (m_board.getSideToMove() == WHITE ? last_score : -last_score);

            std::cout << ", time " << (now_ms() - startTime)
                << ", pv " << best_so_far.toAlgebraic() << std::endl;
        }

        if (last_score > 90000 || last_score < -90000) break;
    }

    std::cout << "Bestmove: " << best_so_far.toAlgebraic() << " score cp " << (m_board.getSideToMove() == WHITE ? last_score : -last_score) << std::endl;
    return best_so_far;
}

Move Searcher::GetBestMove() {
    return IterativeDeepening();
}