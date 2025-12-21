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

const int queenEndgameWeight = 45;
const int rookEndgameWeight = 20;
const int bishopEndgameWeight = 10;
const int knightEndgameWeight = 10;
const int endgameStartWeight = 2 * rookEndgameWeight + 2 * bishopEndgameWeight + 2 * knightEndgameWeight + queenEndgameWeight;

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
    uint64_t myPawns = board.getPieceBitboard(color, PAWN);
    uint64_t enemyPawns = board.getPieceBitboard((Color)(color ^ 1), PAWN);
    Square kingSq = board.getKingSquare(color);

    int bonus = 0;
    int penalty = 0;
    int isolatedCount = 0;

    uint64_t tempPawns = myPawns;
    while (tempPawns) {
        Square sq = PopBit(tempPawns);
        int file = sq % 8;
        int rank = sq / 8;

        uint64_t adjacentFiles = 0;
        if (file > 0) adjacentFiles |= 0x0101010101010101ULL << (file - 1);
        if (file < 7) adjacentFiles |= 0x0101010101010101ULL << (file + 1);

        if (!(myPawns & adjacentFiles)) {
            isolatedCount++;
        }

        uint64_t forwardMask = 0;
        if (color == WHITE) {
            for (int r = rank + 1; r <= 7; r++) {
                forwardMask |= (1ULL << (r * 8 + file));
                if (file > 0) forwardMask |= (1ULL << (r * 8 + file - 1));
                if (file < 7) forwardMask |= (1ULL << (r * 8 + file + 1));
            }
        }
        else {
            for (int r = rank - 1; r >= 0; r--) {
                forwardMask |= (1ULL << (r * 8 + file));
                if (file > 0) forwardMask |= (1ULL << (r * 8 + file - 1));
                if (file < 7) forwardMask |= (1ULL << (r * 8 + file + 1));
            }
        }

        if (!(enemyPawns & forwardMask)) {
            int numSquaresFromPromotion = (color == WHITE) ? 7 - rank : rank;
            bonus += passedPawnBonuses[numSquaresFromPromotion];
        }
    }

    uint64_t infiltratingPawns = enemyPawns;
    while (infiltratingPawns) {
        Square sq = PopBit(infiltratingPawns);
        int file = sq % 8;
        int rank = sq / 8;
        int relativeRank = (color == WHITE) ? rank : (7 - rank);

        if (relativeRank <= 2) {
            int intrusionPenalty = (relativeRank == 1) ? 50 : 25;

            int distToKing = std::abs(file - (kingSq % 8)) + std::abs(rank - (kingSq / 8));
            if (distToKing <= 2) {
                intrusionPenalty += 30;
            }

            penalty += intrusionPenalty;
        }
    }

    return bonus + isolatedPawnPenalty[std::clamp(isolatedCount, 0, 8)];
}

int Evaluation::KingPawnShield(const Board& board, Color color, float enemyEndgameT, float enemyPieceSquareScore) {
    if (enemyEndgameT >= 1.0f) return 0;

    int penalty = 0;
    int uncastledKingPenalty = 0;
    bool isWhite = (color == WHITE);
    Square kingSq = board.getKingSquare(color);
    int kingFile = kingSq % 8;

    Square rookCorner = (kingFile <= 3) ? (isWhite ? A1 : A8) : (isWhite ? H1 : H8);

    if ((kingFile <= 2 || kingFile >= 5) && board.getPieceAt(rookCorner, color) != ROOK) {
        const auto& shieldIndices = isWhite ?
            PrecomputedEvaluationData::PawnShieldSquaresWhite[kingSq] :
            PrecomputedEvaluationData::PawnShieldSquaresBlack[kingSq];

        int numImmediateSquares = (int)shieldIndices.size() / 2;

        for (int i = 0; i < numImmediateSquares; i++) {
            Square shieldSquareIndex = (Square)shieldIndices[i];

            if (board.getPieceAt(shieldSquareIndex, color) != PAWN) {
                if (shieldIndices.size() > 3 && board.getPieceAt((Square)shieldIndices[i + 3], color) == PAWN) {
                    penalty += kingPawnShieldScores[i + 3];
                }
                else {
                    penalty += kingPawnShieldScores[i];
                }
            }
        }
        penalty *= penalty;
    }
    else {
        float enemyDevelopmentScore = std::clamp((enemyPieceSquareScore + 10.0f) / 130.0f, 0.0f, 1.0f);
        uncastledKingPenalty = (int)(50 * enemyDevelopmentScore);
    }

    float pawnShieldWeight = 1.0f - enemyEndgameT;
    if (board.getPieceBitboard((Color)(color ^ 1), QUEEN) == 0) {
        pawnShieldWeight *= 0.6f;
    }

    return (int)((-penalty - uncastledKingPenalty) * pawnShieldWeight);
}

int Evaluation::MopUpEval(const Board& board, Color winner, float endgameT) {
    if (endgameT < 0.5f) return 0;

    int mopUpScore = 0;
    Square myKing = board.getKingSquare(winner);
    Square oppKing = board.getKingSquare((Color)(winner ^ 1));

    int dist = std::abs(myKing / 8 - oppKing / 8) + std::abs(myKing % 8 - oppKing % 8);
    mopUpScore += (14 - dist) * 4;

    int oppKingFile = oppKing % 8;
    int oppKingRank = oppKing / 8;
    int centerDist = std::max(3 - oppKingFile, oppKingFile - 4) + std::max(3 - oppKingRank, oppKingRank - 4);
    mopUpScore += centerDist * 10;

    return (int)(mopUpScore * endgameT);
}

uint64_t Evaluation::GetKingZone(const Board& board, Square kingSq) {
    uint64_t kingBB = (1ULL << kingSq);
    uint64_t zone = board.getKingAttacks(kingSq) | kingBB;

    return zone;
}

int Evaluation::EvaluateKingSafety(const Board& board, Color color) {
    Color enemy = (Color)(color ^ 1);
    Square kingSq = board.getKingSquare(color);
    uint64_t kingZone = GetKingZone(board, kingSq);

    int attackCount = 0;
    int weightSum = 0;

    const int pieceWeights[] = { 0, 2, 2, 3, 5, 0 };

    for (int pt = KNIGHT; pt <= QUEEN; pt++) {
        uint64_t attackers = board.getPieceBitboard(enemy, (PieceType)pt);
        while (attackers) {
            Square sq = PopBit(attackers);
            uint64_t attacks;

            if (pt == KNIGHT) attacks = board.getKnightAttacks(sq);
            else if (pt == BISHOP) attacks = board.getBishopAttacks(sq, board.getAllOccupancy());
            else if (pt == ROOK) attacks = board.getRookAttacks(sq, board.getAllOccupancy());
            else attacks = board.getBishopAttacks(sq, board.getAllOccupancy()) | board.getRookAttacks(sq, board.getAllOccupancy());

            if (attacks & kingZone) {
                attackCount++;
                weightSum += pieceWeights[pt];
            }
        }
    }

    if (attackCount == 0) return 0;

    static const int safetyTable[] = {
        0, 0, 5, 10, 20, 40, 70, 100, 150, 200, 300, 400, 500, 600, 800
    };

    int index = std::clamp(weightSum, 0, 14);
    return safetyTable[index];
}

int Evaluation::EvaluatePos(const Board& board) {
    int mg_score[2] = { 0, 0 };
    int eg_score[2] = { 0, 0 };
    int pst_only_score[2] = { 0, 0 };
    int currentWeightSum = 0;

    for (int color = WHITE; color <= BLACK; color++) {
        Color c = (Color)color;
        for (int pt = PAWN; pt <= KING; pt++) {
            uint64_t bb = board.getPieceBitboard(c, (PieceType)pt);
            while (bb) {
                Square sq = PopBit(bb);
                int eval_sq = (color == WHITE) ? sq : sq ^ 56;

                mg_score[color] += GetPieceValue((PieceType)pt);
                eg_score[color] += GetPieceValue((PieceType)pt);

                if (pt == KNIGHT) currentWeightSum += knightEndgameWeight;
                else if (pt == BISHOP) currentWeightSum += bishopEndgameWeight;
                else if (pt == ROOK) currentWeightSum += rookEndgameWeight;
                else if (pt == QUEEN) currentWeightSum += queenEndgameWeight;

                int mg_pst = 0, eg_pst = 0;
                switch (pt) {
                case PAWN:   mg_pst = pawn_pst[eval_sq]; eg_pst = pawn_pst_eg[eval_sq]; break;
                case KNIGHT: mg_pst = knight_pst[eval_sq]; eg_pst = knight_pst[eval_sq]; break;
                case BISHOP: mg_pst = bishop_pst[eval_sq]; eg_pst = bishop_pst[eval_sq]; break;
                case ROOK:   mg_pst = rook_pst[eval_sq]; eg_pst = rook_pst[eval_sq]; break;
                case QUEEN:  mg_pst = queen_pst[eval_sq]; eg_pst = queen_pst[eval_sq]; break;
                case KING:   mg_pst = king_pst[eval_sq]; eg_pst = king_pst_eg[eval_sq]; break;
                }
                mg_score[color] += mg_pst;
                eg_score[color] += eg_pst;
                pst_only_score[color] += mg_pst;
            }
        }
    }

    float endgameT = 1.0f - std::min(1.0f, (float)currentWeightSum / endgameStartWeight);

    for (int color = WHITE; color <= BLACK; color++) {
        Color c = (Color)color;
        Color enemy = (Color)(color ^ 1);

        mg_score[color] -= EvaluateKingSafety(board, c);

        int pawnEval = EvaluatePawns(board, c);
        mg_score[color] += pawnEval;
        eg_score[color] += pawnEval;

        mg_score[color] += KingPawnShield(board, c, endgameT, (float)pst_only_score[enemy]);

        if (endgameT > 0.7f && mg_score[color] > mg_score[enemy] + 400) {
            int mopUp = MopUpEval(board, c, endgameT);

            eg_score[color] += mopUp;
        }
    }

    int mg_eval = mg_score[WHITE] - mg_score[BLACK];
    int eg_eval = eg_score[WHITE] - eg_score[BLACK];

    int finalEval = (int)(mg_eval * (1.0f - endgameT) + eg_eval * endgameT);

    return (board.getSideToMove() == WHITE) ? finalEval : -finalEval;
}