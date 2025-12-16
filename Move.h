#pragma once
#include <cstdint>
#include "Utils.h"

struct Move {
private:
    uint16_t m_move_data;

public:
    Move() : m_move_data(0) {}
    Move(Square from, Square to, MoveFlag flags = NORMAL_MOVE) {
        m_move_data = (uint16_t)(flags << 12) | (from << 6) | to;
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
};