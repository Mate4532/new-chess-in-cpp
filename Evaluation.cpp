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
    Square kingSq = board.getKingSquare(color);
    uint64_t zone = board.getKingAttacks(kingSq) | (1ULL << kingSq);

    int weight = 0;
    static const int w[] = { 0,2,2,3,5 };

    for (int pt = KNIGHT; pt <= QUEEN; pt++) {
        uint64_t bb = board.getPieceBitboard(enemy, (PieceType)pt);
        while (bb) {
            Square sq = PopBit(bb);
            uint64_t attacks =
                (pt == KNIGHT) ? board.getKnightAttacks(sq) :
                (pt == BISHOP) ? board.getBishopAttacks(sq, board.getAllOccupancy()) :
                (pt == ROOK) ? board.getRookAttacks(sq, board.getAllOccupancy()) :
                board.getBishopAttacks(sq, board.getAllOccupancy()) |
                board.getRookAttacks(sq, board.getAllOccupancy());

            if (attacks & zone)
                weight += w[pt];
        }
    }

    static const int table[] = {
        0,0,5,10,20,40,70,100,150,200,300,400,500,600,800
    };

    return table[std::min(weight, 14)];
}

int Evaluation::KingPawnShield(const Board& board, Color color, float eg, int enemyPst) {
    if (eg > 0.8f) return 0;
    int penalty = enemyPst / 4;
    return -penalty * (1.0f - eg);
}

int Evaluation::MopUpEval(const Board& board, Color winner, float eg) {
    if (eg < 0.6f) return 0;

    Square k1 = board.getKingSquare(winner);
    Square k2 = board.getKingSquare((Color)(winner ^ 1));

    int dist = abs((k1 >> 3) - (k2 >> 3)) + abs((k1 & 7) - (k2 & 7));
    return (14 - dist) * 10 * eg;
}

int Evaluation::RookBlockPenalty(const Board& board, Color color) {
    uint64_t rooks = board.getPieceBitboard(color, ROOK);
    Square kingSq = board.getKingSquare(color);

    int kFile = kingSq & 7;
    int kRank = kingSq >> 3;

    int penalty = 0;

    while (rooks) {
        Square rSq = PopBit(rooks);
        int rFile = rSq & 7;
        int rRank = rSq >> 3;

        if (rFile != kFile)
            continue;

        bool blocked = false;

        int step = (rRank < kRank) ? 1 : -1;
        for (int r = rRank + step; r != kRank; r += step) {
            Square sq = (Square)(r * 8 + rFile);
            if (board.getAllOccupancy() & (1ULL << sq)) {
                blocked = false;
                break;
            }
            blocked = true;
        }

        if (blocked) {
            penalty += 25;
            if (std::abs(kRank - rRank) <= 1)
                penalty += 15;
        }
    }

    return -penalty;
}

int Evaluation::EvaluatePos(const Board& board) {
    int mg[2] = { 0,0 };
    int eg[2] = { 0,0 };
    int pstOnly[2] = { 0,0 };
    int phase = 0;

    for (int c = WHITE; c <= BLACK; c++) {
        for (int pt = PAWN; pt <= KING; pt++) {
            uint64_t bb = board.getPieceBitboard((Color)c, (PieceType)pt);
            while (bb) {
                Square sq = PopBit(bb);
                int s = (c == WHITE) ? sq : sq ^ 56;

                mg[c] += GetPieceValue((PieceType)pt);
                eg[c] += GetPieceValue((PieceType)pt);

                if (pt == KNIGHT) phase += knightWeight;
                else if (pt == BISHOP) phase += bishopWeight;
                else if (pt == ROOK) phase += rookWeight;
                else if (pt == QUEEN) phase += queenWeight;

                int mgpst = 0, egpst = 0;
                if (pt == PAWN) { mgpst = pawn_pst[s]; egpst = pawn_pst_eg[s]; }
                else if (pt == KNIGHT) mgpst = egpst = knight_pst[s];
                else if (pt == BISHOP) mgpst = egpst = bishop_pst[s];
                else if (pt == ROOK) mgpst = egpst = rook_pst[s];
                else if (pt == QUEEN) mgpst = egpst = queen_pst[s];
                else { mgpst = king_pst[s]; egpst = king_pst_eg[s]; }

                mg[c] += mgpst;
                eg[c] += egpst;
                pstOnly[c] += mgpst;
            }
        }
    }

    float egT = 1.0f - std::min(1.0f, (float)phase / endgameStart);

    for (int c = WHITE; c <= BLACK; c++) {
        mg[c] += EvaluatePawns(board, (Color)c);
        eg[c] += EvaluatePawns(board, (Color)c);

        if (egT < 0.6f)
            mg[c] -= EvaluateKingSafety(board, (Color)c);

        mg[c] += KingPawnShield(board, (Color)c, egT, pstOnly[c ^ 1]);

        if (egT > 0.7f && mg[c] > mg[c ^ 1] + 400)
            eg[c] += MopUpEval(board, (Color)c, egT);

        if (egT > 0.5f) {
            mg[c] += RookBlockPenalty(board, (Color)c);
        }
    }

    int score = (int)(mg[WHITE] * (1 - egT) + eg[WHITE] * egT)
        - (int)(mg[BLACK] * (1 - egT) + eg[BLACK] * egT);

    return board.getSideToMove() == WHITE ? score : -score;
}