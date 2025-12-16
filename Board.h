#pragma once
#include "Utils.h"
#include <cstring>
#include <cstdint>
#include "BoardState.h"

class Board {
private:
    uint64_t m_bitboards[2][PIECE_TYPE_COUNT];
    uint64_t m_side_occupancy[2];
    uint64_t m_all_occupancy;

    BoardState history[MAX_PLY];
    uint16_t m_ply;

    BoardState current_state;

    Color m_side_to_move;
    inline int GetSquare(int rank, int file) const { 
        return rank * 8 + file; 
    };

public:
    Board();
    void InitializeBoard();
    void PrintBoard() const;
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
    inline Square getEnPassantSquare() const {
        return (Square)history[m_ply].en_passant_sq;
    }
};
