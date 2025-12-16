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
    m_bitboards[WHITE][QUEEN] = 0b0000000000000000000000000000000000000000000000000000000000010000ULL;
    m_bitboards[WHITE][KING] = 0b0000000000000000000000000000000000000000000000000000000000001000ULL;

    m_bitboards[BLACK][PAWN] = 0b0000000011111111000000000000000000000000000000000000000000000000ULL;

    m_bitboards[BLACK][KNIGHT] = 0b0100001000000000000000000000000000000000000000000000000000000000ULL;
    m_bitboards[BLACK][BISHOP] = 0b0010010000000000000000000000000000000000000000000000000000000000ULL;
    m_bitboards[BLACK][ROOK] = 0b1000000100000000000000000000000000000000000000000000000000000000ULL;
    m_bitboards[BLACK][QUEEN] = 0b0001000000000000000000000000000000000000000000000000000000000000ULL;
    m_bitboards[BLACK][KING] = 0b0000100000000000000000000000000000000000000000000000000000000000ULL;

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
    initialState.en_passant_sq = NONE;
    initialState.castling_rights = ALL_CASTLING_RIGHTS;
    initialState.half_move_clock = 0;
    initialState.full_move_number = 1;

    history[m_ply] = initialState;

    std::vector v = MoveGenerator::GenerateMoves(*this);
    printf("%zu", v.size());
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

