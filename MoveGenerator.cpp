#include <vector>
#include "Board.h"
#include "Move.h"
#include "MoveGenerator.h"

std::vector<Move> MoveGenerator::GenerateMoves(const Board& board) {
    std::vector<Move> moves;

    Color player = board.getSideToMove();
    Color enemy = (player == WHITE) ? BLACK : WHITE;

    uint64_t all_occupancy = board.getAllOccupancy();

    for (int p = PAWN; p < PIECE_TYPE_COUNT; p++) {
        PieceType piece_type = (PieceType)p;

        uint64_t piece_bb = board.getPieceBitboard(player, piece_type);

        uint64_t enemy_occupancy = board.getSideOccupancy(enemy);

        switch (piece_type) {
        case PAWN: {
            if (player == WHITE) {

                //Normal pawn moves

                uint64_t white_pawns = piece_bb;
                uint64_t push_targets = (white_pawns << 8);
                    
                push_targets &= ~all_occupancy;

                while (push_targets) {
                    Square to_sq = PopBit(push_targets);
                    Square from_sq = (Square)(to_sq - 8);

                    AddPawnMove(moves, from_sq, to_sq, WHITE, NORMAL_MOVE);
                }

                //Double pawn moves

                uint64_t rank2_pawns = white_pawns & RANK_2;
                uint64_t intermediate_squares = (rank2_pawns << 8) & ~all_occupancy;

                uint64_t double_push_targets = (intermediate_squares << 8) & ~all_occupancy;

                while (double_push_targets) {
                    Square to_sq = PopBit(double_push_targets);
                    Square from_sq = (Square)(to_sq - 16);

                    moves.push_back(Move(from_sq, to_sq, DOUBLE_PAWN_PUSH));
                }

                //Capture pawn moves (NorthWest and NorthEast)

                uint64_t nw_pawns = white_pawns & (~FILE_A);
                uint64_t nw_targets = (nw_pawns << 7) & enemy_occupancy;

                uint64_t ne_pawns = white_pawns & (~FILE_H);
                uint64_t ne_targets = (ne_pawns << 9) & enemy_occupancy;

                uint64_t capture_targets = nw_targets | ne_targets;

                while (capture_targets) {
                    Square to_sq = PopBit(capture_targets);

                    Square from_sq_nw = (Square)(to_sq - 7);
                    if ((1ULL << from_sq_nw) & white_pawns) {
                        AddPawnMove(moves, from_sq_nw, to_sq, WHITE, CAPTURE);
                    }

                    Square from_sq_ne = (Square)(to_sq - 9);
                    if ((1ULL << from_sq_ne) & white_pawns) {
                        AddPawnMove(moves, from_sq_ne, to_sq, WHITE, CAPTURE);
                    }
                }

                //En passant move

                Square ep_sq = board.getEnPassantSquare();

                if (ep_sq != NO_SQUARE) {
                    uint64_t ep_target = 1ULL << ep_sq;

                    uint64_t nw_attacker = (ep_target >> 9) & white_pawns & RANK_5;

                    uint64_t ne_attacker = (ep_target >> 7) & white_pawns & RANK_5;

                    uint64_t attackers = nw_attacker | ne_attacker;

                    while (attackers) {
                        Square from_sq = PopBit(attackers);

                        moves.push_back(Move(from_sq, ep_sq, EN_PASSANT));
                    }
                }

            }
            else {

                //Normal pawn moves

                uint64_t black_pawns = piece_bb;
                uint64_t push_targets = (black_pawns >> 8);

                push_targets &= ~all_occupancy;

                while (push_targets) {
                    Square to_sq = PopBit(push_targets);
                    Square from_sq = (Square)(to_sq + 8);

                    AddPawnMove(moves, from_sq, to_sq, BLACK, NORMAL_MOVE);
                }

                //Double pawn moves

                uint64_t rank7_pawns = black_pawns & RANK_7;
                uint64_t intermediate_squares = (rank7_pawns >> 8) & ~all_occupancy;

                uint64_t double_push_targets = (intermediate_squares >> 8) & ~all_occupancy;

                while (double_push_targets) {
                    Square to_sq = PopBit(double_push_targets);
                    Square from_sq = (Square)(to_sq + 16);

                    moves.push_back(Move(from_sq, to_sq, DOUBLE_PAWN_PUSH));
                }

                //Capture pawn moves (SouthWest and SouthEast)

                uint64_t sw_pawns = black_pawns & (~FILE_H);
                uint64_t sw_targets = (sw_pawns >> 7) & enemy_occupancy;

                uint64_t se_pawns = black_pawns & (~FILE_A);
                uint64_t se_targets = (se_pawns >> 9) & enemy_occupancy;

                uint64_t capture_targets = sw_targets | se_targets;

                while (capture_targets) {
                    Square to_sq = PopBit(capture_targets);

                    Square from_sq_sw = (Square)(to_sq + 7);
                    if ((1ULL << from_sq_sw) & black_pawns) {
                        AddPawnMove(moves, from_sq_sw, to_sq, BLACK, CAPTURE);
                    }

                    Square from_sq_se = (Square)(to_sq + 9);
                    if ((1ULL << from_sq_se) & black_pawns) {
                        AddPawnMove(moves, from_sq_se, to_sq, BLACK, CAPTURE);
                    }
                }

                //En passant move

                Square ep_sq = board.getEnPassantSquare();

                if (ep_sq != NO_SQUARE) {
                    uint64_t ep_target = 1ULL << ep_sq;

                    uint64_t sw_attacker = (ep_target << 7) & black_pawns & RANK_4;

                    uint64_t se_attacker = (ep_target << 9) & black_pawns & RANK_4;

                    uint64_t attackers = sw_attacker | se_attacker;

                    while (attackers) {
                        Square from_sq = PopBit(attackers);

                        moves.push_back(Move(from_sq, ep_sq, EN_PASSANT));
                    }
                }

            }
            break;
        }

        case KNIGHT: {

            break;
        }

        case BISHOP: {

            break;
        }

        case ROOK: {

            break;
        }

        case QUEEN: {

            break;
        }

        case KING: {

            break;
        }

        default:
            break;
        }
    }

    return moves;
}