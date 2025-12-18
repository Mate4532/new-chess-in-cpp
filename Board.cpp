#include "Board.h"
#include <cstdint>
#include <iostream>
#include "MoveGenerator.h"
#include <chrono>
#include <thread>

uint64_t pawn_attacks_table[2][64];
uint64_t knight_attacks_table[64];
uint64_t king_attacks_table[64];

std::atomic<uint64_t> global_node_count(0);

Board::Board() {
    InitializeBoard();
}

void Board::InitializeBoard() {
    LoadFEN("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10 ");
    InitializeAttackTables();
}

void Board::InitializeAttackTables() {
    for (int sq = 0; sq < 64; sq++) {
        uint64_t b = (1ULL << sq);

        uint64_t k_attacks = 0;
        k_attacks |= (b << 17) & ~FILE_A;
        k_attacks |= (b << 15) & ~FILE_H;
        k_attacks |= (b << 10) & ~(FILE_A | FILE_B);
        k_attacks |= (b << 6) & ~(FILE_G | FILE_H);
        k_attacks |= (b >> 17) & ~FILE_H;
        k_attacks |= (b >> 15) & ~FILE_A;
        k_attacks |= (b >> 10) & ~(FILE_G | FILE_H);
        k_attacks |= (b >> 6) & ~(FILE_A | FILE_B);
        knight_attacks_table[sq] = k_attacks;

        uint64_t ki_attacks = 0;
        ki_attacks |= (b << 8);
        ki_attacks |= (b >> 8);
        ki_attacks |= (b << 1) & ~FILE_A;
        ki_attacks |= (b >> 1) & ~FILE_H;
        ki_attacks |= (b << 7) & ~FILE_H;
        ki_attacks |= (b << 9) & ~FILE_A;
        ki_attacks |= (b >> 7) & ~FILE_A;
        ki_attacks |= (b >> 9) & ~FILE_H;
        king_attacks_table[sq] = ki_attacks;

        uint64_t w_pawn = 0;
        w_pawn |= (b << 7) & ~FILE_H;
        w_pawn |= (b << 9) & ~FILE_A;
        pawn_attacks_table[WHITE][sq] = w_pawn;

        uint64_t b_pawn = 0;
        b_pawn |= (b >> 7) & ~FILE_A;
        b_pawn |= (b >> 9) & ~FILE_H;
        pawn_attacks_table[BLACK][sq] = b_pawn;
    }
}

void Board::LoadFEN(std::string fen) {
    if (fen.empty()) {
        fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    }

    for (int c = 0; c < 2; c++) {
        m_side_occupancy[c] = 0ULL;
        for (int p = PAWN; p <= KING; p++) {
            m_bitboards[c][p] = 0ULL;
        }
    }
    m_all_occupancy = 0ULL;
    m_ply = 0;

    std::stringstream ss(fen);
    std::string pieces, side, castling, enPassant, halfMove, fullMove;
    ss >> pieces >> side >> castling >> enPassant >> halfMove >> fullMove;

    int rank = 7;
    int file = 0;
    for (char c : pieces) {
        if (c == '/') {
            rank--;
            file = 0;
        }
        else if (isdigit(c)) {
            file += (c - '0');
        }
        else {
            Color color = isupper(c) ? WHITE : BLACK;
            PieceType type;
            char lowerC = tolower(c);
            if (lowerC == 'p') type = PAWN;
            else if (lowerC == 'n') type = KNIGHT;
            else if (lowerC == 'b') type = BISHOP;
            else if (lowerC == 'r') type = ROOK;
            else if (lowerC == 'q') type = QUEEN;
            else if (lowerC == 'k') type = KING;

            Square sq = (Square)(rank * 8 + file);
            m_bitboards[color][type] |= (1ULL << sq);
            file++;
        }
    }

    m_side_to_move = (side == "w") ? WHITE : BLACK;

    BoardState state;
    state.castling_rights = 0;
    if (castling != "-") {
        for (char c : castling) {
            if (c == 'K') state.castling_rights |= WHITE_KINGSIDE_CASTLE;
            else if (c == 'Q') state.castling_rights |= WHITE_QUEENSIDE_CASTLE;
            else if (c == 'k') state.castling_rights |= BLACK_KINGSIDE_CASTLE;
            else if (c == 'q') state.castling_rights |= BLACK_QUEENSIDE_CASTLE;
        }
    }

    if (enPassant == "-") {
        state.en_passant_sq = SQUARE_NONE;
    }
    else {
        int f = enPassant[0] - 'a';
        int r = enPassant[1] - '1';
        state.en_passant_sq = (Square)(r * 8 + f);
    }

    state.half_move_clock = halfMove.empty() ? 0 : stoi(halfMove);
    state.full_move_number = fullMove.empty() ? 1 : stoi(fullMove);
    state.captured_piece_type = PIECE_NONE;

    for (int c = 0; c < 2; c++) {
        for (int p = PAWN; p <= KING; p++) {
            m_side_occupancy[c] |= m_bitboards[c][p];
        }
    }
    m_all_occupancy = m_side_occupancy[WHITE] | m_side_occupancy[BLACK];

    boardStateHistory[m_ply] = state;
}

PieceType Board::getPieceAt(Square sq, Color color) const {
    for (int piece = PAWN; piece <= KING; piece++) {
        if (m_bitboards[color][piece] & (1ULL << sq)) {
            return (PieceType)piece;
        }
    }
    return PIECE_NONE;
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
    attacks |= (bit << 1) & ~FILE_A;
    attacks |= (bit >> 1) & ~FILE_H;

    attacks |= (bit << 7) & ~FILE_H;
    attacks |= (bit << 9) & ~FILE_A;
    attacks |= (bit >> 7) & ~FILE_A;
    attacks |= (bit >> 9) & ~FILE_H;

    return attacks;
}

uint64_t Board::getInvertedPawnAttacks(Square sq, Color attackerColor) const {

    return pawn_attacks_table[attackerColor ^ 1][sq];
}

bool Board::isSquareAttacked(Square sq, Color attackerColor) const {
    if (pawn_attacks_table[attackerColor ^ 1][sq] & m_bitboards[attackerColor][PAWN]) return true;

    if (knight_attacks_table[sq] & m_bitboards[attackerColor][KNIGHT]) return true;
    if (king_attacks_table[sq] & m_bitboards[attackerColor][KING]) return true;

    uint64_t occ = m_all_occupancy;
    if (getBishopAttacks(sq, occ) & (m_bitboards[attackerColor][BISHOP] | m_bitboards[attackerColor][QUEEN])) return true;
    if (getRookAttacks(sq, occ) & (m_bitboards[attackerColor][ROOK] | m_bitboards[attackerColor][QUEEN])) return true;

    return false;
}

bool Board::MakeMove(Move move) {
    Square from_sq = move.getFrom();
    Square to_sq = move.getTo();
    MoveFlag flags = move.getFlags();
    Color player = m_side_to_move;
    Color enemy = (player == WHITE) ? BLACK : WHITE;
    PieceType piece = move.getPieceType();

    BoardState newBoardState;
    newBoardState.captured_piece_type = PIECE_NONE;
    newBoardState.en_passant_sq = SQUARE_NONE;
    newBoardState.castling_rights = boardStateHistory[m_ply].castling_rights;
    newBoardState.half_move_clock = boardStateHistory[m_ply].half_move_clock + 1;
    newBoardState.full_move_number = boardStateHistory[m_ply].full_move_number + (player == BLACK ? 1 : 0);

    if (flags & CAPTURE_FLAG) {
        newBoardState.half_move_clock = 0;
        if (flags == EN_PASSANT) {
            Square cap_sq = (player == WHITE) ? (Square)(to_sq - 8) : (Square)(to_sq + 8);
            newBoardState.captured_piece_type = PAWN;
            m_bitboards[enemy][PAWN] ^= (1ULL << cap_sq);
            m_side_occupancy[enemy] ^= (1ULL << cap_sq);
        }
        else {
            PieceType captured = getPieceAt(to_sq, enemy);;
            newBoardState.captured_piece_type = captured;
            m_bitboards[enemy][captured] ^= (1ULL << to_sq);
            m_side_occupancy[enemy] ^= (1ULL << to_sq);
        }
    }

    if (piece == PAWN) newBoardState.half_move_clock = 0;

    bool is_promotion = (flags & PROMOTION_FLAG);

    if (is_promotion) {
        m_bitboards[player][PAWN] ^= (1ULL << from_sq);
        m_side_occupancy[player] ^= (1ULL << from_sq);

        PieceType prom_piece;

        uint8_t promType = flags & 0b0011;
        if (promType == 0b0011) prom_piece = QUEEN;
        else if (promType == 0b0010) prom_piece = ROOK;
        else if (promType == 0b0001) prom_piece = BISHOP;
        else prom_piece = KNIGHT;

        m_bitboards[player][prom_piece] ^= (1ULL << to_sq);
        m_side_occupancy[player] ^= (1ULL << to_sq);
    }
    else {
        m_bitboards[player][piece] ^= (1ULL << from_sq) | (1ULL << to_sq);
        m_side_occupancy[player] ^= (1ULL << from_sq) | (1ULL << to_sq);

        if (flags == DOUBLE_PAWN_PUSH) {
            newBoardState.en_passant_sq = (player == WHITE) ? (Square)(from_sq + 8) : (Square)(from_sq - 8);
        }
        else if (flags == KINGSIDE_CASTLE) {
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
    }

    if (piece == KING) {
        newBoardState.castling_rights &= (player == WHITE ? ~WHITE_ALL_CASTLE_RIGHTS : ~BLACK_ALL_CASTLE_RIGHTS);
    }

    newBoardState.castling_rights &= castling_mask[from_sq];
    newBoardState.castling_rights &= castling_mask[to_sq];

    m_all_occupancy = m_side_occupancy[WHITE] | m_side_occupancy[BLACK];
    m_ply++;
    boardStateHistory[m_ply] = newBoardState;
    m_side_to_move = enemy;

    Square kingSq = (Square)GetLSB(m_bitboards[player][KING]);
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
    PieceType piece_type = move.getPieceType();

    BoardState& state_to_undo = boardStateHistory[m_ply];
    PieceType captured = (PieceType)state_to_undo.captured_piece_type;

    m_side_to_move = (m_side_to_move == WHITE) ? BLACK : WHITE;
    Color player = m_side_to_move;
    Color enemy = (player == WHITE) ? BLACK : WHITE;

    if (flags & PROMOTION_FLAG) {

        m_bitboards[player][piece_type] ^= (1ULL << to_sq);
        m_bitboards[player][PAWN] ^= (1ULL << from_sq);
        m_side_occupancy[player] ^= (1ULL << from_sq) | (1ULL << to_sq);
    }

    else {
        m_bitboards[player][piece_type] ^= (1ULL << to_sq) | (1ULL << from_sq);
        m_side_occupancy[player] ^= (1ULL << to_sq) | (1ULL << from_sq);

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
    }

    if (flags & CAPTURE_FLAG) {
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

    m_all_occupancy = m_side_occupancy[WHITE] | m_side_occupancy[BLACK];
    m_ply--;
}

uint64_t Board::PerftDivide(int depth) {
    std::vector<Move> moves = MoveGenerator::GenerateMoves(*this);
    uint64_t total_nodes = 0;
    for (const Move& move : moves) {
        if (!MakeMove(move)) {
            continue;
        }
        uint64_t nodes = Perft(depth - 1);
        std::cout << square_to_coordinates[(int)move.getFrom()] << " -> " << square_to_coordinates[(int)move.getTo()] << ": " << nodes << std::endl;
        total_nodes += nodes;

        UndoMove(move);
    }
    return total_nodes;
}

uint64_t Board::Perft(int depth) {
    if (depth == 0) return 1ULL;

    std::vector<Move> moves = MoveGenerator::GenerateMoves(*this);

    uint64_t nodes = 0;

    for (const Move& move : moves) {
        if (!MakeMove(move)) {
            continue;
        }

        nodes += Perft(depth - 1);

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

void perft_thread_worker(Board board_copy, std::vector<Move> moves_to_test, int depth) {
    uint64_t local_nodes = 0;

    for (const auto& move : moves_to_test) {
        if (board_copy.MakeMove(move)) {
            local_nodes += board_copy.Perft(depth - 1);
            board_copy.UndoMove(move);
        }
    }

    global_node_count += local_nodes;
}

uint64_t Board::MultiThreadedPerft(int depth) {
    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<Move> root_moves = MoveGenerator::GenerateMoves(*this);

    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 4;

    std::vector<std::thread> threads;
    std::vector<std::vector<Move>> move_chunks(num_threads);

    for (size_t i = 0; i < root_moves.size(); ++i) {
        move_chunks[i % num_threads].push_back(root_moves[i]);
    }

    global_node_count = 0;
    std::cout << "Inditas " << num_threads << " szalon..." << std::endl;

    for (int i = 0; i < num_threads; ++i) {
        if (move_chunks[i].empty()) continue;

        threads.emplace_back(perft_thread_worker, *this, move_chunks[i], depth);
    }

    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    uint64_t nodes = global_node_count;
    double nps = nodes / elapsed.count();

    return global_node_count;
}