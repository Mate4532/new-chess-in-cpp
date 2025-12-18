#pragma once
#include "Utils.h"
#include <cstring>
#include <cstdint>
#include "BoardState.h"
#include "Move.h"
#include <memory>
#include <vector>
#include <sstream>
#include <atomic>

extern uint64_t pawn_attacks_table[2][64];
extern uint64_t knight_attacks_table[64];
extern uint64_t king_attacks_table[64];

class Board {
private:
    uint64_t m_bitboards[2][PIECE_TYPE_COUNT];
    uint64_t m_side_occupancy[2];
    uint64_t m_all_occupancy;

    BoardState boardStateHistory[1024];
    uint16_t m_ply;

    BoardState current_state;

    Color m_side_to_move;
    inline int GetSquare(int rank, int file) const { 
        return rank * 8 + file; 
    };

public:
    Board();
    void InitializeBoard();
    void InitializeAttackTables();
    void LoadFEN(std::string fen);
    PieceType getPieceAt(Square sq, Color color) const;
    uint64_t getBishopAttacks(Square sq, uint64_t occupied) const;
    uint64_t getKnightAttacks(Square sq) const;
    uint64_t getRookAttacks(Square sq, uint64_t occupied) const;
    uint64_t getKingAttacks(Square sq) const;
    uint64_t getInvertedPawnAttacks(Square sq, Color attackerColor) const;
    inline const uint64_t(&getBitboards() const)[2][PIECE_TYPE_COUNT]{
        return m_bitboards;
    }
    inline uint64_t getPieceBitboard(Color c, PieceType p) const {
        return m_bitboards[c][p];
    }
    inline Color getSideToMove() const {
        return m_side_to_move;
    }
    inline uint64_t getSideOccupancy(Color c) const {
        return m_side_occupancy[c];
    }
    inline uint64_t getAllOccupancy() const {
        return m_all_occupancy;
    }
    inline CastlingRight getCastlingRights() const{
        return (CastlingRight)boardStateHistory[m_ply].castling_rights;
    }
    inline Square getEnPassantSquare() const {
        return (Square)boardStateHistory[m_ply].en_passant_sq;
    }
    inline PieceType getCapturePieceType() const{
        return (PieceType)boardStateHistory[m_ply].captured_piece_type;
    }
    inline uint8_t getHalfMoveClock() const {
        return boardStateHistory[m_ply].half_move_clock;
    }
    inline uint16_t getFullMoveNumber() const {
        return boardStateHistory[m_ply].full_move_number;
    }
    bool isSquareAttacked(Square sq, Color attackerColor) const;
    bool MakeMove(Move move);
    void UndoMove(Move move);
    uint64_t PerftDivide(int depth);
    uint64_t Perft(int depth);
    uint64_t MultiThreadedPerft(int depth);
    void PrintBoard() const;
};
