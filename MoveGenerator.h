#pragma once
#include <vector>
#include "Move.h"
#include "Board.h"

class MoveGenerator {
private:
    static void GeneratePawnMoves(const Board& board, Color player, uint64_t piece_bb, uint64_t enemy_occupancy, uint64_t all_occupancy, std::vector<Move>& moves);
    static void GenerateKnightMoves(const Board& board, Color player, uint64_t piece_bb, uint64_t enemy_occupancy, std::vector<Move>& moves);
    static void GenerateBishopMoves(const Board& board, Color player, uint64_t piece_bb, uint64_t enemy_occupancy, uint64_t all_occupancy, std::vector<Move>& moves);
    static void GenerateRookMoves(const Board& board, Color player, uint64_t piece_bb, uint64_t enemy_occupancy, uint64_t all_occupancy, std::vector<Move>& moves);
    static void GenerateKingMoves(const Board& board, Color player, uint64_t piece_bb, uint64_t enemy_occupancy, uint64_t all_occupancy, std::vector<Move>& moves);

    static inline void AddPawnMove(std::vector<Move>& moves, Square from_sq, Square to_sq, Color pawn_color, MoveFlag flag) {
        if (pawn_color == WHITE ? to_sq >= A8 : to_sq <= H1) {
            moves.push_back(Move(from_sq, to_sq, (MoveFlag)(flag | PROMOTION_TYPE_KNIGHT)));
            moves.push_back(Move(from_sq, to_sq, (MoveFlag)(flag | PROMOTION_TYPE_BISHOP)));
            moves.push_back(Move(from_sq, to_sq, (MoveFlag)(flag | PROMOTION_TYPE_ROOK)));
            moves.push_back(Move(from_sq, to_sq, (MoveFlag)(flag | PROMOTION_TYPE_QUEEN)));
        }
        else {
            moves.push_back(Move(from_sq, to_sq, flag));
        }
    }


public:
    static std::vector<Move> GenerateMoves(const Board& board);
};