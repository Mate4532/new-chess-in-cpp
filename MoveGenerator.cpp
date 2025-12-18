#include <vector>
#include "Board.h"
#include "Move.h"
#include "MoveGenerator.h"
#include "Attacks.h"

std::vector<Move> MoveGenerator::GenerateMoves(const Board& board) {
    std::vector<Move> moves;
    moves.reserve(256);

    Color player = board.getSideToMove();
    Color enemy = (player == WHITE) ? BLACK : WHITE;

    uint64_t all_occ = board.getAllOccupancy();
    uint64_t enemy_occ = board.getSideOccupancy(enemy);
    uint64_t friendly_occ = board.getSideOccupancy(player);

    for (int p = PAWN; p <= KING; p++) {
        PieceType piece_type = (PieceType)p;
        uint64_t piece_bb = board.getPieceBitboard(player, piece_type);

        while (piece_bb) {
            Square from_sq = PopBit(piece_bb);

            switch (piece_type) {
                case PAWN: {
                    int direction = (player == WHITE) ? 8 : -8;
                    Square to_sq = (Square)(from_sq + direction);

                    if (!((1ULL << to_sq) & all_occ)) {
                        AddPawnMove(moves, from_sq, to_sq, player, NORMAL_MOVE);

                        if ((player == WHITE && from_sq / 8 == 1) || (player == BLACK && from_sq / 8 == 6)) {
                            Square double_to = (Square)(to_sq + direction);
                            if (!((1ULL << double_to) & all_occ)) {
                                moves.push_back(Move(from_sq, double_to, PAWN, DOUBLE_PAWN_PUSH));
                            }
                        }
                    }

                    uint64_t attack_mask = board.getInvertedPawnAttacks(from_sq, enemy);
                    uint64_t captures = attack_mask & enemy_occ;
                    while (captures) {
                        Square cap_to = PopBit(captures);
                        AddPawnMove(moves, from_sq, cap_to, player, CAPTURE);
                    }

                    Square ep_sq = board.getEnPassantSquare();
                    if (ep_sq != SQUARE_NONE && (attack_mask & (1ULL << ep_sq))) {
                        moves.push_back(Move(from_sq, ep_sq, PAWN, EN_PASSANT));
                    }
                    break;
                }

                case KNIGHT: {
                    uint64_t knight_attacks = board.getKnightAttacks(from_sq) & ~friendly_occ;
                    while (knight_attacks) {
                        Square to_sq = PopBit(knight_attacks);
                        MoveFlag flag = ((1ULL << to_sq) & enemy_occ) ? CAPTURE : NORMAL_MOVE;
                        moves.push_back(Move(from_sq, to_sq, KNIGHT, flag));
                    }
                    break;
                }

                case BISHOP: {
                    uint64_t bishop_attacks = board.getBishopAttacks(from_sq, all_occ) & ~friendly_occ;
                    while (bishop_attacks) {
                        Square to_sq = PopBit(bishop_attacks);
                        MoveFlag flag = ((1ULL << to_sq) & enemy_occ) ? CAPTURE : NORMAL_MOVE;
                        moves.push_back(Move(from_sq, to_sq, BISHOP, flag));
                    }
                    break;
                }

                case ROOK: {
                    uint64_t rook_attacks = board.getRookAttacks(from_sq, all_occ) & ~friendly_occ;
                    while (rook_attacks) {
                        Square to_sq = PopBit(rook_attacks);
                        MoveFlag flag = ((1ULL << to_sq) & enemy_occ) ? CAPTURE : NORMAL_MOVE;
                        moves.push_back(Move(from_sq, to_sq, ROOK, flag));
                    }
                    break;
                }

                case QUEEN: {
                    uint64_t queen_attacks = (board.getRookAttacks(from_sq, all_occ) |
                        board.getBishopAttacks(from_sq, all_occ)) & ~friendly_occ;
                    while (queen_attacks) {
                        Square to_sq = PopBit(queen_attacks);
                        MoveFlag flag = ((1ULL << to_sq) & enemy_occ) ? CAPTURE : NORMAL_MOVE;
                        moves.push_back(Move(from_sq, to_sq, QUEEN, flag));
                    }
                    break;
                }

                case KING: {
                    uint64_t king_attacks = board.getKingAttacks(from_sq) & ~friendly_occ;
                    while (king_attacks) {
                        Square to_sq = PopBit(king_attacks);
                        MoveFlag flag = ((1ULL << to_sq) & enemy_occ) ? CAPTURE : NORMAL_MOVE;
                        moves.push_back(Move(from_sq, to_sq, KING, flag));
                    }

                    CastlingRight rights = board.getCastlingRights();
                    if (!board.isSquareAttacked(from_sq, enemy)) {
                        if (player == WHITE) {
                            if ((rights & WHITE_KINGSIDE_CASTLE) && !(all_occ & 0x60ULL)) {
                                if (!board.isSquareAttacked(F1, BLACK) && !board.isSquareAttacked(G1, BLACK))
                                    moves.push_back(Move(E1, G1, KING, KINGSIDE_CASTLE));
                            }
                            if ((rights & WHITE_QUEENSIDE_CASTLE) && !(all_occ & 0x0EULL)) {
                                if (!board.isSquareAttacked(D1, BLACK) && !board.isSquareAttacked(C1, BLACK))
                                    moves.push_back(Move(E1, C1, KING, QUEENSIDE_CASTLE));
                            }
                        }
                        else {
                            if ((rights & BLACK_KINGSIDE_CASTLE) && !(all_occ & 0x6000000000000000ULL)) {
                                if (!board.isSquareAttacked(F8, WHITE) && !board.isSquareAttacked(G8, WHITE))
                                    moves.push_back(Move(E8, G8, KING, KINGSIDE_CASTLE));
                            }
                            if ((rights & BLACK_QUEENSIDE_CASTLE) && !(all_occ & 0x0E00000000000000ULL)) {
                                if (!board.isSquareAttacked(D8, WHITE) && !board.isSquareAttacked(C8, WHITE))
                                    moves.push_back(Move(E8, C8, KING, QUEENSIDE_CASTLE));
                            }
                        }
                    }
                    break;
                }
            }
        }
    }

    return moves;
}