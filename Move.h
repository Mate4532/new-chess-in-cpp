#pragma once
#include <cstdint>
#include "Utils.h"

struct Move {
private:
    uint16_t m_move_data;
    uint8_t m_piece_type;

public:
    Move() : m_move_data(0), m_piece_type(0) {}
    Move(Square from, Square to, PieceType piece_type, MoveFlag flags = NORMAL_MOVE) {
        m_move_data = (uint16_t)(flags << 12) | (from << 6) | to;
        m_piece_type = piece_type;
    }

    inline Square getFrom() const {
        return (Square)((m_move_data >> 6) & 0x3F);
    }

    inline Square getTo() const {
        return (Square)(m_move_data & 0x3F);
    }

    inline MoveFlag getFlags() const {
        return (MoveFlag)(m_move_data >> 12);
    }

    inline PieceType getPieceType() const {
        return (PieceType)m_piece_type;
    }
};