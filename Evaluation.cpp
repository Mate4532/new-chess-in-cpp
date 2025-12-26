#include "Evaluation.h"
#include "PrecomputedEvaluationData.h"

const int pawnValue = 100;
const int knightValue = 300;
const int bishopValue = 320;
const int rookValue = 500;
const int queenValue = 900;

const int passedPawnBonuses[] = { 0, 120, 80, 50, 30, 15, 15 };
const int isolatedPawnPenalty[] = { 0, -10, -25, -50, -75, -75, -75, -75, -75 };
const int kingPawnShieldScores[] = { 4, 7, 4, 3, 6, 3 };

static const int knightWeight = 10;
static const int bishopWeight = 10;
static const int rookWeight = 20;
static const int queenWeight = 45;
static const int endgameStart = 2 * knightWeight + 2 * bishopWeight + 2 * rookWeight + queenWeight;

static const uint64_t CENTER_4 = (1ULL << D4) | (1ULL << E4) | (1ULL << D5) | (1ULL << E5);
static const uint64_t CENTER_16 =
    (RANK_3 | RANK_4 | RANK_5 | RANK_6) &
    (FILE_C | FILE_D | FILE_E | FILE_F);

int Evaluation::GetPieceValue(PieceType p) {
    switch (p) {
    case PAWN:   return 100;
    case KNIGHT: return 320;
    case BISHOP: return 330;
    case ROOK:   return 500;
    case QUEEN:  return 900;
    case KING:   return 10000;
    default:     return 0;
    }
}

int Evaluation::EvaluatePawnTerritory(const Board& board, Color color) {
    uint64_t pawns = board.getPieceBitboard(color, PAWN);
    int bonus = 0;

    while (pawns) {
        Square sq = PopBit(pawns);
        int rank = sq >> 3;
        int file = sq & 7;

        bool inEnemyTerritory = (color == WHITE) ? (rank >= 4) : (rank <= 3);
        if (!inEnemyTerritory) continue;

        bonus += 8;
        if (file >= 2 && file <= 5) bonus += 4;

        uint64_t attacks = board.getPawnAttacks(sq, color);
        uint64_t enemyTerritoryMask =
            (color == WHITE)
            ? (RANK_5 | RANK_6 | RANK_7 | RANK_8)
            : (RANK_1 | RANK_2 | RANK_3 | RANK_4);

        int controlledCount = std::popcount(attacks & enemyTerritoryMask);
        bonus += std::min(controlledCount * 2, 6);
    }
    return bonus;
}

int Evaluation::MaterialImbalancePenalty(PieceType lost, int pawnsGained, float egT) {
    if (lost == KNIGHT || lost == BISHOP) {

        if (pawnsGained >= 3) {
            float mgWeight = 1.0f - egT;

            int penalty = 60;

            return -(int)(penalty * mgWeight);
        }
    }

    return 0;
}

int Evaluation::EvaluatePawnCenter(const Board& board, Color color) {
    uint64_t pawns = board.getPieceBitboard(color, PAWN);
    int score = 0;

    score += 12 * std::popcount(pawns & CENTER_4);
    score += 4 * std::popcount(pawns & CENTER_16);

    return score;
}

int Evaluation::EvaluatePawns(const Board& board, Color color) {
    uint64_t pawns = board.getPieceBitboard(color, PAWN);
    uint64_t enemyPawns = board.getPieceBitboard((Color)(color ^ 1), PAWN);

    int score = 0;
    int isolated = 0;

    uint64_t bb = pawns;
    while (bb) {
        Square sq = PopBit(bb);
        int file = sq & 7;
        int rank = sq >> 3;

        uint64_t adjacent = 0;
        if (file > 0) adjacent |= FILE_MASKS[file - 1];
        if (file < 7) adjacent |= FILE_MASKS[file + 1];

        if (!(pawns & adjacent))
            isolated++;

        uint64_t blockMask = 0;
        if (color == WHITE) {
            for (int r = rank + 1; r < 8; r++)
                blockMask |= (1ULL << (r * 8 + file));
        }
        else {
            for (int r = rank - 1; r >= 0; r--)
                blockMask |= (1ULL << (r * 8 + file));
        }

        if (!(enemyPawns & blockMask)) {
            int dist = color == WHITE ? 7 - rank : rank;
            score += passedPawnBonuses[std::clamp(dist, 0, 6)];
        }
    }

    score += isolatedPawnPenalty[std::clamp(isolated, 0, 8)];
    return score;
}

int Evaluation::EvaluateKingSafety(const Board& board, Color color) {
    Color enemy = (Color)(color ^ 1);

    if (board.getPieceBitboard(enemy, QUEEN) == 0 && board.getPieceBitboard(enemy, ROOK) == 0) return 0;

    Square kingSq = board.getKingSquare(color);
    uint64_t zone = board.getKingAttacks(kingSq) | (1ULL << kingSq);
    int weight = 0;
    const int w[] = { 0, 2, 2, 3, 5, 0 };

    for (int pt = KNIGHT; pt <= QUEEN; pt++) {
        uint64_t bb = board.getPieceBitboard(enemy, (PieceType)pt);
        while (bb) {
            Square sq = PopBit(bb);
            uint64_t attacks = (pt == KNIGHT) ? board.getKnightAttacks(sq) :
                (pt == BISHOP) ? board.getBishopAttacks(sq, board.getAllOccupancy()) :
                (pt == ROOK) ? board.getRookAttacks(sq, board.getAllOccupancy()) :
                (board.getBishopAttacks(sq, board.getAllOccupancy()) | board.getRookAttacks(sq, board.getAllOccupancy()));

            if (attacks & zone) weight += w[pt];
        }
    }

    static const int table[] = { 0, 0, 10, 20, 40, 80, 130, 190, 260, 350, 460, 580, 720, 900, 1100 };
    return table[std::min(weight, 14)];
}

int Evaluation::KingPawnShield(const Board& board, Color color, float egT) {
    if (egT > 0.7f) return 0;   

    Square kingSq = board.getKingSquare(color);
    int kFile = kingSq & 7;
    uint64_t myPawns = board.getPieceBitboard(color, PAWN);
    int shieldBonus = 0;

    for (int file = std::max(0, kFile - 1); file <= std::min(7, kFile + 1); file++) {
        uint64_t fileMask = 0x0101010101010101ULL << file;
        uint64_t shieldRanks = (color == WHITE) ? (RANK_2 | RANK_3) : (RANK_7 | RANK_6);

        if (myPawns & fileMask & shieldRanks) {
            shieldBonus += 15;
        }
    }
    return (int)(shieldBonus * (1.0f - egT));
}

int Evaluation::MopUpEval(const Board& board, Color winner, float eg) {
    if (eg < 0.6f) return 0;

    Square k1 = board.getKingSquare(winner);
    Square k2 = board.getKingSquare((Color)(winner ^ 1));

    int dist = abs((k1 >> 3) - (k2 >> 3)) + abs((k1 & 7) - (k2 & 7));
    return (14 - dist) * 10 * eg;
}

int Evaluation::RookBlockPenalty(const Board& board, Color color) {
    Square kingSq = board.getKingSquare(color);
    int kFile = kingSq & 7;
    int kRank = kingSq >> 3;
    int penalty = 0;

    if (kFile <= 2 || kFile >= 5) {
        uint64_t rooks = board.getPieceBitboard(color, ROOK);
        while (rooks) {
            Square rSq = PopBit(rooks);
            int rFile = rSq & 7;
			int rRank = rSq >> 3;

            if ((rFile == 0 && kFile < 3 && kFile > 0 && rRank == kRank) || (rFile == 7 && kFile > 4 && kFile < 7 && rRank == kRank)) {

                uint64_t attacks = board.getRookAttacks(rSq, board.getAllOccupancy());
                if (std::popcount(attacks) <= 3) {
                    penalty += 40;
                }
            }
        }
    }
    return -penalty;
}

int Evaluation::EvaluatePos(const Board& board) {
    int mg[2] = { 0, 0 };
    int eg[2] = { 0, 0 };
    int totalPhase = 0;
    int pieceCounts[2][6] = { {0} };

    for (int c = WHITE; c <= BLACK; c++) {
        for (int pt = PAWN; pt <= KING; pt++) {
            uint64_t bb = board.getPieceBitboard((Color)c, (PieceType)pt);
            int count = std::popcount(bb);
            if (count == 0) continue;

            pieceCounts[c][pt] = count;
            int val = GetPieceValue((PieceType)pt);

            if (pt == KNIGHT)      totalPhase += count * knightWeight;
            else if (pt == BISHOP) totalPhase += count * bishopWeight;
            else if (pt == ROOK)   totalPhase += count * rookWeight;
            else if (pt == QUEEN)  totalPhase += count * queenWeight;

            mg[c] += count * val;
            eg[c] += count * val;

            while (bb) {
                Square sq = PopBit(bb);
                int s = (c == WHITE) ? sq : sq ^ 56;

                if (pt == PAWN) { mg[c] += pawn_pst[s]; eg[c] += pawn_pst_eg[s]; }
                else if (pt == KNIGHT) { mg[c] += knight_pst[s]; eg[c] += knight_pst[s]; }
                else if (pt == BISHOP) { mg[c] += bishop_pst[s]; eg[c] += bishop_pst[s]; }
                else if (pt == ROOK) { mg[c] += rook_pst[s];   eg[c] += rook_pst[s]; }
                else if (pt == QUEEN) { mg[c] += queen_pst[s];  eg[c] += queen_pst[s]; }
                else { mg[c] += king_pst[s];   eg[c] += king_pst_eg[s]; }
            }
        }
    }

    float egT = 1.0f - std::min(1.0f, (float)totalPhase / endgameStart);

    for (int c = WHITE; c <= BLACK; c++) {
        Color us = (Color)c;
        Color opp = (Color)(c ^ 1);

        if (pieceCounts[opp][KNIGHT] > pieceCounts[us][KNIGHT])
            mg[c] += MaterialImbalancePenalty(KNIGHT, pieceCounts[us][PAWN] - pieceCounts[opp][PAWN], egT);
        if (pieceCounts[opp][BISHOP] > pieceCounts[us][BISHOP])
            mg[c] += MaterialImbalancePenalty(BISHOP, pieceCounts[us][PAWN] - pieceCounts[opp][PAWN], egT);

        if (pieceCounts[us][BISHOP] >= 2) { mg[c] += 30; eg[c] += 50; }

        mg[c] += EvaluatePawns(board, us);
        eg[c] += EvaluatePawns(board, us);
        mg[c] += EvaluatePawnTerritory(board, us);

        if (egT < 0.6f) mg[c] -= EvaluateKingSafety(board, us);
        mg[c] += KingPawnShield(board, us, egT);

        if (egT < 0.7f) mg[c] += EvaluatePawnCenter(board, us);
        if (egT > 0.5f) mg[c] += RookBlockPenalty(board, us);

        if (egT > 0.7f && mg[c] > mg[opp] + 400)
            eg[c] += MopUpEval(board, us, egT);
    }

    int score = ((mg[WHITE] - mg[BLACK]) * (256 - totalPhase) + (eg[WHITE] - eg[BLACK]) * totalPhase) / 256;

    return (board.getSideToMove() == WHITE) ? score : -score;
}