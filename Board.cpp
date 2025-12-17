#include "Board.h"
#include <cstdint>
#include <iostream>
#include "MoveGenerator.h"

Board::Board() {
    InitializeBoard();
}

void Board::InitializeBoard() {
    m_bitboards[WHITE][PAWN] = 0b0000000000000000000000000000000000000000000000001111111100000000ULL;

    m_bitboards[WHITE][KNIGHT] = 0b0000000000000000000000000000000000000000000000000000000001000010ULL;
    m_bitboards[WHITE][BISHOP] = 0b0000000000000000000000000000000000000000000000000000000000100100ULL;
    m_bitboards[WHITE][ROOK] = 0b0000000000000000000000000000000000000000000000000000000010000001ULL;
    m_bitboards[WHITE][QUEEN] = 0b0000000000000000000000000000000000000000000000000000000000001000ULL;
    m_bitboards[WHITE][KING] = 0b0000000000000000000000000000000000000000000000000000000000010000ULL;

    m_bitboards[BLACK][PAWN] = 0b0000000011111111000000000000000000000000000000000000000000000000ULL;

    m_bitboards[BLACK][KNIGHT] = 0b0100001000000000000000000000000000000000000000000000000000000000ULL;
    m_bitboards[BLACK][BISHOP] = 0b0010010000000000000000000000000000000000000000000000000000000000ULL;
    m_bitboards[BLACK][ROOK] = 0b1000000100000000000000000000000000000000000000000000000000000000ULL;
    m_bitboards[BLACK][QUEEN] = 0b0000100000000000000000000000000000000000000000000000000000000000ULL;
    m_bitboards[BLACK][KING] = 0b0001000000000000000000000000000000000000000000000000000000000000ULL;

    m_side_occupancy[WHITE] =
        m_bitboards[WHITE][PAWN] |
        m_bitboards[WHITE][KNIGHT] |
        m_bitboards[WHITE][BISHOP] |
        m_bitboards[WHITE][ROOK] |
        m_bitboards[WHITE][QUEEN] |
        m_bitboards[WHITE][KING];

    m_side_occupancy[BLACK] =
        m_bitboards[BLACK][PAWN] |
        m_bitboards[BLACK][KNIGHT] |
        m_bitboards[BLACK][BISHOP] |
        m_bitboards[BLACK][ROOK] |
        m_bitboards[BLACK][QUEEN] |
        m_bitboards[BLACK][KING];

    m_all_occupancy = m_side_occupancy[WHITE] | m_side_occupancy[BLACK];

    m_side_to_move = WHITE;
    m_ply = 0;
    
    BoardState initialState;
    initialState.en_passant_sq = SQUARE_NONE;
    initialState.castling_rights = ALL_CASTLING_RIGHTS;
    initialState.half_move_clock = 0;
    initialState.full_move_number = 1;

    boardStateHistory[m_ply] = initialState;
}

uint64_t Board::getRookAttacks(Square sq, uint64_t occupied) const {
    uint64_t attacks = 0;
    int dr[] = { 1, -1, 0, 0 };
    int df[] = { 0, 0, 1, -1 };

    int r = sq / 8;
    int f = sq % 8;

    for (int i = 0; i < 4; i++) {
        for (int step = 1; step < 8; step++) {
            int nr = r + dr[i] * step;
            int nf = f + df[i] * step;
            if (nr < 0 || nr > 7 || nf < 0 || nf > 7) break;

            Square target = (Square)(nr * 8 + nf);
            attacks |= (1ULL << target);
            if (occupied & (1ULL << target)) break;
        }
    }
    return attacks;
}

uint64_t Board::getKnightAttacks(Square s) const {
    uint64_t b = 1ULL << s;
    uint64_t attacks = 0;
    attacks |= (b << 17) & ~FILE_A;
    attacks |= (b << 15) & ~FILE_H;
    attacks |= (b << 10) & ~(FILE_A | FILE_B);
    attacks |= (b << 6) & ~(FILE_G | FILE_H);

    attacks |= (b >> 17) & ~FILE_H;
    attacks |= (b >> 15) & ~FILE_A;
    attacks |= (b >> 10) & ~(FILE_G | FILE_H);
    attacks |= (b >> 6) & ~(FILE_A | FILE_B);

    return attacks;
}

uint64_t Board::getBishopAttacks(Square sq, uint64_t occupied) const {
    uint64_t attacks = 0;

    int dr[] = { 1, 1, -1, -1 };
    int df[] = { 1, -1, 1, -1 };

    int r = sq / 8;
    int f = sq % 8;

    for (int i = 0; i < 4; i++) {
        for (int step = 1; step < 8; step++) {
            int nr = r + dr[i] * step;
            int nf = f + df[i] * step;

            if (nr < 0 || nr > 7 || nf < 0 || nf > 7) break;

            Square target = (Square)(nr * 8 + nf);
            attacks |= (1ULL << target);

            if (occupied & (1ULL << target)) break;
        }
    }
    return attacks;
}

uint64_t Board::getKingAttacks(Square sq) const {
    uint64_t bit = (1ULL << sq);
    uint64_t attacks = 0;

    attacks |= (bit << 8);
    attacks |= (bit >> 8);
    attacks |= (bit << 1) & ~0x0101010101010101ULL;
    attacks |= (bit >> 1) & ~0x8080808080808080ULL;

    attacks |= (bit << 7) & ~0x8080808080808080ULL;
    attacks |= (bit << 9) & ~0x0101010101010101ULL;
    attacks |= (bit >> 7) & ~0x0101010101010101ULL;
    attacks |= (bit >> 9) & ~0x8080808080808080ULL;

    return attacks;
}

uint64_t Board::getInvertedPawnAttacks(Square sq, Color attackerColor) const {
    uint64_t bit = (1ULL << sq);
    uint64_t attacks = 0;
    if (attackerColor == WHITE) {
        attacks |= (bit >> 7) & ~0x0101010101010101ULL;
        attacks |= (bit >> 9) & ~0x8080808080808080ULL;
    }
    else {
        attacks |= (bit << 7) & ~0x8080808080808080ULL;
        attacks |= (bit << 9) & ~0x0101010101010101ULL;
    }
    return attacks;
}

PieceType Board::getPieceAt(Square sq) const {

    PieceType whitePiece = getPieceAt(sq, WHITE);
    if (whitePiece != PIECE_NONE) return whitePiece;

    return getPieceAt(sq, BLACK);
}

PieceType Board::getPieceAt(Square sq, Color color) const {

    for (int piece = PAWN; piece <= KING; piece++) {
        if (m_bitboards[color][piece] & (1ULL << sq)) {
            return (PieceType)piece;
        }
    }

    return PIECE_NONE;
}

bool Board::isSquareAttacked(Square sq, Color attackerColor) const {

    uint64_t pawns = m_bitboards[attackerColor][PAWN];
    if (getInvertedPawnAttacks(sq, (Color)!attackerColor) & pawns) return true;

    if (getKnightAttacks(sq) & m_bitboards[attackerColor][KNIGHT]) return true;

    if (getKingAttacks(sq) & m_bitboards[attackerColor][KING]) return true;

    uint64_t occupied = m_all_occupancy;
    if (getBishopAttacks(sq, occupied) & (m_bitboards[attackerColor][BISHOP] | m_bitboards[attackerColor][QUEEN])) return true;
    if (getRookAttacks(sq, occupied) & (m_bitboards[attackerColor][ROOK] | m_bitboards[attackerColor][QUEEN])) return true;

    return false;
}

bool Board::MakeMove(Move move) {
    Square from_sq = move.getFrom();
    Square to_sq = move.getTo();
    MoveFlag flags = move.getFlags();
    Color player = m_side_to_move;
    Color enemy = (player == WHITE) ? BLACK : WHITE;
    PieceType piece = getPieceAt(from_sq, player);

    BoardState newBoardState;
    newBoardState.captured_piece_type = PIECE_NONE;
    newBoardState.en_passant_sq = SQUARE_NONE;
    newBoardState.castling_rights = getCastlingRights();
    newBoardState.half_move_clock = getHalfMoveClock() + 1;
    newBoardState.full_move_number = getFullMoveNumber() + (player == BLACK ? 1 : 0);

    if (flags & CAPTURE) {
        if (flags == EN_PASSANT) {
            Square cap_sq = (player == WHITE) ? (Square)(to_sq - 8) : (Square)(to_sq + 8);
            newBoardState.captured_piece_type = PAWN;
            m_bitboards[enemy][PAWN] ^= (1ULL << cap_sq);
            m_side_occupancy[enemy] ^= (1ULL << cap_sq);
        }
        else {
            PieceType captured = getPieceAt(to_sq, enemy);
            newBoardState.captured_piece_type = captured;
            m_bitboards[enemy][captured] ^= (1ULL << to_sq);
            m_side_occupancy[enemy] ^= (1ULL << to_sq);
        }
        newBoardState.half_move_clock = 0;
    }

    if (piece == PAWN) newBoardState.half_move_clock = 0;

    bool is_promotion = false;

    switch (flags) {
        case DOUBLE_PAWN_PUSH:
            newBoardState.en_passant_sq = (player == WHITE) ? (Square)(from_sq + 8) : (Square)(from_sq - 8);
            break;

        case KINGSIDE_CASTLE: {
            Square r_from = (player == WHITE) ? H1 : H8;
            Square r_to = (player == WHITE) ? F1 : F8;
            m_bitboards[player][ROOK] ^= (1ULL << r_from) | (1ULL << r_to);
            m_side_occupancy[player] ^= (1ULL << r_from) | (1ULL << r_to);
            newBoardState.castling_rights &= (player == WHITE ? ~WHITE_ALL_CASTLE_RIGHTS : ~BLACK_ALL_CASTLE_RIGHTS);
            break;
        }
        case QUEENSIDE_CASTLE: {
            Square r_from = (player == WHITE) ? A1 : A8;
            Square r_to = (player == WHITE) ? D1 : D8;
            m_bitboards[player][ROOK] ^= (1ULL << r_from) | (1ULL << r_to);
            m_side_occupancy[player] ^= (1ULL << r_from) | (1ULL << r_to);
            newBoardState.castling_rights &= (player == WHITE ? ~WHITE_ALL_CASTLE_RIGHTS : ~BLACK_ALL_CASTLE_RIGHTS);
            break;
        }
        case EN_PASSANT: {
            Square cap_sq = (Square)((player == WHITE) ? (to_sq - 8) : (to_sq + 8));
            m_bitboards[enemy][PAWN] ^= (1ULL << cap_sq);
            m_side_occupancy[enemy] ^= (1ULL << cap_sq);
            newBoardState.captured_piece_type = PAWN;
            break;
        }

        case PROMOTION_TYPE_QUEEN:
        case PROMOTION_TYPE_ROOK:
        case PROMOTION_TYPE_BISHOP:
        case PROMOTION_TYPE_KNIGHT: {
            is_promotion = true;
            PieceType prom_piece;
            if (flags == PROMOTION_TYPE_QUEEN) prom_piece = QUEEN;
            else if (flags == PROMOTION_TYPE_ROOK) prom_piece = ROOK;
            else if (flags == PROMOTION_TYPE_BISHOP) prom_piece = BISHOP;
            else prom_piece = KNIGHT;


            m_bitboards[player][PAWN] ^= (1ULL << from_sq);
            m_bitboards[player][prom_piece] ^= (1ULL << to_sq);
            m_side_occupancy[player] ^= (1ULL << from_sq) | (1ULL << to_sq);
            break;
        }
    }

    if (!is_promotion) {
        m_bitboards[player][piece] ^= (1ULL << from_sq) | (1ULL << to_sq);
        m_side_occupancy[player] ^= (1ULL << from_sq) | (1ULL << to_sq);
    }

    if (piece == KING) {
        newBoardState.castling_rights &= (player == WHITE ? ~WHITE_ALL_CASTLE_RIGHTS : ~BLACK_ALL_CASTLE_RIGHTS);
    }
    if (from_sq == A1) newBoardState.castling_rights &= ~WHITE_QUEENSIDE_CASTLE;
    if (from_sq == H1) newBoardState.castling_rights &= ~WHITE_KINGSIDE_CASTLE;
    if (from_sq == A8) newBoardState.castling_rights &= ~BLACK_QUEENSIDE_CASTLE;
    if (from_sq == H8) newBoardState.castling_rights &= ~BLACK_KINGSIDE_CASTLE;

    m_all_occupancy = m_side_occupancy[WHITE] | m_side_occupancy[BLACK];

    m_ply++;
    boardStateHistory[m_ply] = newBoardState;
    m_side_to_move = enemy;

    uint64_t kingBB = m_bitboards[player][KING];
    Square kingSq = (Square)GetLSB(kingBB);

    if (isSquareAttacked(kingSq, enemy)) {

        UndoMove(move);
        return false;
    }

    return true;
}

void Board::UndoMove(Move move) {
    Square from_sq = move.getFrom();
    Square to_sq = move.getTo();
    MoveFlag flags = move.getFlags();

    BoardState& state_to_undo = boardStateHistory[m_ply];
    PieceType captured = (PieceType)state_to_undo.captured_piece_type;

    m_side_to_move = (m_side_to_move == WHITE) ? BLACK : WHITE;
    Color player = m_side_to_move;
    Color enemy = (player == WHITE) ? BLACK : WHITE;

    if (flags & (PROMOTION_TYPE_QUEEN | PROMOTION_TYPE_ROOK | PROMOTION_TYPE_BISHOP | PROMOTION_TYPE_KNIGHT)) {
        PieceType prom_piece = getPieceAt(to_sq, player);
        m_bitboards[player][prom_piece] ^= (1ULL << to_sq);
        m_bitboards[player][PAWN] ^= (1ULL << from_sq);
        m_side_occupancy[player] ^= (1ULL << from_sq) | (1ULL << to_sq);
    }
    else {
        PieceType piece = getPieceAt(to_sq, player);
        m_bitboards[player][piece] ^= (1ULL << to_sq) | (1ULL << from_sq);
        m_side_occupancy[player] ^= (1ULL << to_sq) | (1ULL << from_sq);
    }

    if (flags & CAPTURE) {
        if (flags == EN_PASSANT) {
            Square cap_sq = (player == WHITE) ? (Square)(to_sq - 8) : (Square)(to_sq + 8);
            m_bitboards[enemy][PAWN] ^= (1ULL << cap_sq);
            m_side_occupancy[enemy] ^= (1ULL << cap_sq);
        }
        else {
            m_bitboards[enemy][captured] ^= (1ULL << to_sq);
            m_side_occupancy[enemy] ^= (1ULL << to_sq);
        }
    }

    if (flags == KINGSIDE_CASTLE) {
        Square r_from = (player == WHITE) ? H1 : H8;
        Square r_to = (player == WHITE) ? F1 : F8;
        m_bitboards[player][ROOK] ^= (1ULL << r_from) | (1ULL << r_to);
        m_side_occupancy[player] ^= (1ULL << r_from) | (1ULL << r_to);
    }
    else if (flags == QUEENSIDE_CASTLE) {
        Square r_from = (player == WHITE) ? A1 : A8;
        Square r_to = (player == WHITE) ? D1 : D8;
        m_bitboards[player][ROOK] ^= (1ULL << r_from) | (1ULL << r_to);
        m_side_occupancy[player] ^= (1ULL << r_from) | (1ULL << r_to);
    }

    m_all_occupancy = m_side_occupancy[WHITE] | m_side_occupancy[BLACK];
    m_ply--;
}

uint64_t Board::Perft(int depth, bool useBulk) {
    if (depth == 0) return 1ULL;

    std::vector<Move> moves = MoveGenerator::GenerateMoves(*this);
    if (useBulk && depth == 1) {
        return (uint64_t)moves.size();
    }

    uint64_t nodes = 0;

    for (const Move& move : moves) {
        if (!MakeMove(move)) {
            continue;
        }

        nodes += Perft(depth - 1, useBulk);

        UndoMove(move);
    }

    return nodes;
}

void Board::PrintBoard() const {
    std::cout << "\n    +-----------------+" << std::endl;

    for (int rank = 7; rank >= 0; rank--)
    {
        std::cout << "  " << (rank + 1) << " |";

        for (int file = 0; file < 8; file++)
        {
            int sq = GetSquare(rank, file);

            char piece_to_print = '.';
            bool found = false;

            for (int color = 0; color < 2; color++)
            {
                for (int piece_type = PAWN; piece_type <= KING; piece_type++)
                {
                    uint64_t current_bb = m_bitboards[color][piece_type];

                    if (current_bb & (1ULL << sq))
                    {
                        piece_to_print = piece_chars[color][piece_type];
                        found = true;
                        break;
                    }
                }
                if (found) {
                    break;
                }
            }

            std::cout << " " << piece_to_print;
        }
        std::cout << " |" << std::endl;
    }

    std::cout << "    +-----------------+" << std::endl;
    std::cout << "      A B C D E F G H\n" << std::endl;

    std::cout << "Jatekban levo szin: " << ((m_side_to_move == WHITE) ? "Feher" : "Fekete") << std::endl;
}

