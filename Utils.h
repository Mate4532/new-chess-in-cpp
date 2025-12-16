#pragma once
#include <cstdint>

#define MAX_PLY 2048

enum Color : uint8_t {
    WHITE = 0,
    BLACK = 1,
    COLORS = 2
};

enum PieceType : uint8_t {
    PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
    PIECE_TYPE_COUNT, NONE
};

enum Square : uint8_t {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    SQUARE_COUNT = 64,
    NO_SQUARE
};

const char piece_chars[2][7] = {
    {'P', 'N', 'B', 'R', 'Q', 'K', '.'},
    {'p', 'n', 'b', 'r', 'q', 'k', '.'}
};

enum CastlingRights : uint8_t {
    WHITE_KINGSIDE_CASTLE = 0b0001,
    WHITE_QUEENSIDE_CASTLE = 0b0010,
    BLACK_KINGSIDE_CASTLE = 0b0100,
    BLACK_QUEENSIDE_CASTLE = 0b1000,
    ALL_CASTLING_RIGHTS = WHITE_KINGSIDE_CASTLE | WHITE_QUEENSIDE_CASTLE | BLACK_KINGSIDE_CASTLE | BLACK_QUEENSIDE_CASTLE
};

enum MoveFlag : uint8_t {
    NORMAL_MOVE = 0,
    DOUBLE_PAWN_PUSH = 0b0001,
    KING_CASTLE = 0b0010,
    QUEEN_CASTLE = 0b0011,

    CAPTURE = 0b0100,
    EN_PASSANT = 0b0101,

    PROMOTION_SHIFT = 4,

    PROMOTION_TYPE_KNIGHT = 0b0001 << PROMOTION_SHIFT,
    PROMOTION_TYPE_BISHOP = 0b0010 << PROMOTION_SHIFT,
    PROMOTION_TYPE_ROOK = 0b0011 << PROMOTION_SHIFT,
    PROMOTION_TYPE_QUEEN = 0b0100 << PROMOTION_SHIFT,

    ACTION_MASK = 0b00001111,
    PROMOTION_MASK = 0b11110000
};

inline Square GetLSB(uint64_t bb) {
#ifdef _MSC_VER
    unsigned long index;
    _BitScanForward64(&index, bb);
    return (Square)index;
#else
    return (Square)__builtin_ctzll(bb);
#endif
}

inline Square PopBit(uint64_t& bb) {
    Square sq = GetLSB(bb);
    bb &= bb - 1;
    return sq;
}

const uint64_t RANK_1 = 0x00000000000000FFULL;
const uint64_t RANK_2 = 0x000000000000FF00ULL;
const uint64_t RANK_3 = 0x0000000000FF0000ULL;
const uint64_t RANK_4 = 0x00000000FF000000ULL;
const uint64_t RANK_5 = 0x000000FF00000000ULL;
const uint64_t RANK_6 = 0x0000FF0000000000ULL;
const uint64_t RANK_7 = 0x00FF000000000000ULL;
const uint64_t RANK_8 = 0xFF00000000000000ULL;

const uint64_t FILE_A = 0x0101010101010101ULL;
const uint64_t FILE_B = 0x0202020202020202ULL;
const uint64_t FILE_C = 0x0404040404040404ULL;
const uint64_t FILE_D = 0x0808080808080808ULL;
const uint64_t FILE_E = 0x1010101010101010ULL;
const uint64_t FILE_F = 0x2020202020202020ULL;
const uint64_t FILE_G = 0x4040404040404040ULL;
const uint64_t FILE_H = 0x8080808080808080ULL;