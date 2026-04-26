#include "oldsearcher.h"
#include "OldEvaluation.h"
#include "OldWorseEvaluation.h"

#include <chrono>
#include <iostream>
#include <random>

using namespace OldEvaluation;
using namespace OldSearcher;
using namespace OldMoveOrdering;

inline long long now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

void Searcher::setTournamentTime(long long timeLeftMs, long long incrementMs) {
    this->timeLeftMs = timeLeftMs;
    this->incrementMs = incrementMs;
}

void Searcher::stopSearch() {
    stop = true;
    isStoppedManually = true;
}

int Searcher::quiescence(int alpha, int beta, int ply) {
    nodes++;

    if ((nodes & 511) == 0 && now_ms() - startTime >= hardTimeLimit)
        stop = true;
    if (stop) return alpha;

    int standPat = currentSettings.worseEvaluationEnabled ? OldWorseEvaluation::Evaluation::EvaluatePos(board) : OldEvaluation::Evaluation::EvaluatePos(board, ply, nnue_state);

    if (standPat >= beta) {
        return beta;
    }

    if (standPat > alpha) {
        alpha = standPat;
    }

    MoveList moves;
    MoveGenerator::GenerateMoves(board, moves, true);

    MoveOrdering::SortMoves(board, moves);

    for (const Move& m : moves) {

        nnue_state[ply + 1].dirtyPiece.dirtyNum = 0;
        nnue_state[ply + 1].accumulator.computedAccumulation = 0;

        MoveFlag flags = m.getFlags();
        bool isCapture = flags & CAPTURE_FLAG;
        bool isPromo = flags & PROMOTION_FLAG;

        Color player = board.getSideToMove();
        Color enemy = (Color)(player ^ 1);
        PieceType mPieceType = m.getPieceType();
        Square mFrom = m.getFrom();
        Square mTo = m.getTo();

        bool isEp = flags == EN_PASSANT;

        if (isEp) {
            nnue_state[ply + 1].dirtyPiece.dirtyNum = 2;

            nnue_state[ply + 1].dirtyPiece.pc[0] = Evaluation::GetNnuePieceNum(PAWN, player);
            nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
            nnue_state[ply + 1].dirtyPiece.to[0] = mTo;

            int epSquare = mTo + (player == WHITE ? MoveGenerator::WHITE_ENPASSANT_PIECE_OFFSET : MoveGenerator::BLACK_ENPASSANT_PIECE_OFFSET);
            nnue_state[ply + 1].dirtyPiece.pc[1] = Evaluation::GetNnuePieceNum(PAWN, enemy);
            nnue_state[ply + 1].dirtyPiece.from[1] = epSquare;
            nnue_state[ply + 1].dirtyPiece.to[1] = 64;
        }
        else if (isCapture) {
            if (isPromo) {
                PieceType promoPiece = Board::GetPromotionPiece(flags);
                PieceType capPiece = board.getPieceAt(mTo, enemy);

                nnue_state[ply + 1].dirtyPiece.dirtyNum = 3;

                nnue_state[ply + 1].dirtyPiece.pc[0] = Evaluation::GetNnuePieceNum(PAWN, player);
                nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
                nnue_state[ply + 1].dirtyPiece.to[0] = 64;

                nnue_state[ply + 1].dirtyPiece.pc[1] = Evaluation::GetNnuePieceNum(capPiece, enemy);
                nnue_state[ply + 1].dirtyPiece.from[1] = mTo;
                nnue_state[ply + 1].dirtyPiece.to[1] = 64;

                nnue_state[ply + 1].dirtyPiece.pc[2] = Evaluation::GetNnuePieceNum(promoPiece, player);
                nnue_state[ply + 1].dirtyPiece.from[2] = 64;
                nnue_state[ply + 1].dirtyPiece.to[2] = mTo;
            }

            else {
                PieceType capPiece = board.getPieceAt(mTo, enemy);
                nnue_state[ply + 1].dirtyPiece.dirtyNum = 2;

                nnue_state[ply + 1].dirtyPiece.pc[0] = Evaluation::GetNnuePieceNum(mPieceType, player);
                nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
                nnue_state[ply + 1].dirtyPiece.to[0] = mTo;

                nnue_state[ply + 1].dirtyPiece.pc[1] = Evaluation::GetNnuePieceNum(capPiece, enemy);
                nnue_state[ply + 1].dirtyPiece.from[1] = mTo;
                nnue_state[ply + 1].dirtyPiece.to[1] = 64;
            }
        }

        if (!board.MakeMove(m, true)) continue;

        int score = -quiescence(-beta, -alpha, ply + 1);
        board.UndoMove(m, true);

        if (stop) return alpha;
        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
            alpha = score;
        }
    }
    return alpha;
}

int Searcher::negamax(int depth, int alpha, int beta, int ply) {

    nodes++;
    bool isPvNode = (beta - alpha > 1);

    if ((nodes & 511) == 0 && now_ms() - startTime >= hardTimeLimit)
        stop = true;
    if (stop)
        return alpha;

    int originalAlpha = alpha;
    uint64_t hash = board.getHash();
    if (ply > 0) {
        if (board.getHalfMoveClock() >= 100 || repetitionTable.Contains(hash) || board.IsInsufficientMaterial()) {
            return 0;
        }
        alpha = std::max(alpha, -MATE_SCORE + ply);
        beta = std::min(beta, MATE_SCORE - ply);
        if (alpha >= beta) return alpha;
    }

    MoveList moves;
    MoveGenerator::GenerateMoves(board, moves);

    bool inCheck = board.isSquareAttacked(board.getKingSquare(board.getSideToMove()), (Color)(board.getSideToMove() ^ 1));

    if (inCheck) {
        depth++;
    }

    if (depth <= 0) return quiescence(alpha, beta, ply);

    MoveOrdering::SortMoves(
        board,
        moves
    );

    Move bestMove;
    int movesSearched = 0;

    for (const Move& m : moves) {

        nnue_state[ply + 1].dirtyPiece.dirtyNum = 0;
        nnue_state[ply + 1].accumulator.computedAccumulation = 0;

        MoveFlag flags = m.getFlags();
        bool isCapture = flags & CAPTURE_FLAG;
        bool isPromo = flags & PROMOTION_FLAG;
        bool quiet = !isCapture && !isPromo;
        bool isCastle = (flags == KINGSIDE_CASTLE || flags == QUEENSIDE_CASTLE);
        bool isEp = flags == EN_PASSANT;

        Color player = board.getSideToMove();
        Color enemy = (Color)(player ^ 1);
        PieceType mPieceType = m.getPieceType();
        Square mFrom = m.getFrom();
        Square mTo = m.getTo();

        if (isCapture) {

            if (isEp) {
                nnue_state[ply + 1].dirtyPiece.dirtyNum = 2;

                nnue_state[ply + 1].dirtyPiece.pc[0] = Evaluation::GetNnuePieceNum(PAWN, player);
                nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
                nnue_state[ply + 1].dirtyPiece.to[0] = mTo;

                int epSquare = mTo + (player == WHITE ? MoveGenerator::WHITE_ENPASSANT_PIECE_OFFSET : MoveGenerator::BLACK_ENPASSANT_PIECE_OFFSET);
                nnue_state[ply + 1].dirtyPiece.pc[1] = Evaluation::GetNnuePieceNum(PAWN, enemy);
                nnue_state[ply + 1].dirtyPiece.from[1] = epSquare;
                nnue_state[ply + 1].dirtyPiece.to[1] = 64;
            }

            else {
                nnue_state[ply + 1].dirtyPiece.pc[0] = Evaluation::GetNnuePieceNum(mPieceType, player);
                nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
                nnue_state[ply + 1].dirtyPiece.to[0] = isPromo ? 64 : mTo;

                PieceType capPiece = board.getPieceAt(mTo, enemy);
                nnue_state[ply + 1].dirtyPiece.pc[1] = Evaluation::GetNnuePieceNum(capPiece, enemy);
                nnue_state[ply + 1].dirtyPiece.from[1] = mTo;
                nnue_state[ply + 1].dirtyPiece.to[1] = 64;

                if (isPromo) {
                    PieceType promoPiece = Board::GetPromotionPiece(flags);
                    nnue_state[ply + 1].dirtyPiece.dirtyNum = 3;

                    nnue_state[ply + 1].dirtyPiece.pc[2] = Evaluation::GetNnuePieceNum(promoPiece, player);
                    nnue_state[ply + 1].dirtyPiece.from[2] = 64;
                    nnue_state[ply + 1].dirtyPiece.to[2] = mTo;
                }

                else {
                    nnue_state[ply + 1].dirtyPiece.dirtyNum = 2;
                }
            }
        }

        else if (isPromo) {
            nnue_state[ply + 1].dirtyPiece.dirtyNum = 2;

            nnue_state[ply + 1].dirtyPiece.pc[0] = Evaluation::GetNnuePieceNum(PAWN, player);
            nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
            nnue_state[ply + 1].dirtyPiece.to[0] = 64;

            PieceType promoPiece = Board::GetPromotionPiece(flags);
            nnue_state[ply + 1].dirtyPiece.pc[1] = Evaluation::GetNnuePieceNum(promoPiece, player);
            nnue_state[ply + 1].dirtyPiece.from[1] = 64;
            nnue_state[ply + 1].dirtyPiece.to[1] = mTo;
        }

        else if (isCastle) {
            nnue_state[ply + 1].dirtyPiece.dirtyNum = 2;
            if (flags & KINGSIDE_CASTLE) {
                nnue_state[ply + 1].dirtyPiece.pc[0] = Evaluation::GetNnuePieceNum(KING, player);
                nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
                nnue_state[ply + 1].dirtyPiece.to[0] = mTo;

                nnue_state[ply + 1].dirtyPiece.pc[1] = Evaluation::GetNnuePieceNum(ROOK, player);
                nnue_state[ply + 1].dirtyPiece.from[1] = player == WHITE ? MoveGenerator::WHITE_KINGSIDE_CASTLE_ROOK_POS_FROM : MoveGenerator::BLACK_KINGSIDE_CASTLE_ROOK_POS_FROM;
                nnue_state[ply + 1].dirtyPiece.to[1] = player == WHITE ? MoveGenerator::WHITE_KINGSIDE_CASTLE_ROOK_POS_TO : MoveGenerator::BLACK_KINGSIDE_CASTLE_ROOK_POS_TO;

            }

            else if (flags & QUEENSIDE_CASTLE) {
                nnue_state[ply + 1].dirtyPiece.dirtyNum = 2;
                nnue_state[ply + 1].dirtyPiece.pc[0] = Evaluation::GetNnuePieceNum(KING, player);
                nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
                nnue_state[ply + 1].dirtyPiece.to[0] = mTo;

                nnue_state[ply + 1].dirtyPiece.pc[1] = Evaluation::GetNnuePieceNum(ROOK, player);
                nnue_state[ply + 1].dirtyPiece.from[1] = player == WHITE ? MoveGenerator::WHITE_QUEENSIDE_CASTLE_ROOK_POS_FROM : MoveGenerator::BLACK_QUEENSIDE_CASTLE_ROOK_POS_FROM;
                nnue_state[ply + 1].dirtyPiece.to[1] = player == WHITE ? MoveGenerator::WHITE_QUEENSIDE_CASTLE_ROOK_POS_TO : MoveGenerator::BLACK_QUEENSIDE_CASTLE_ROOK_POS_TO;
            }
        }

        else {
            nnue_state[ply + 1].dirtyPiece.dirtyNum = 1;

            nnue_state[ply + 1].dirtyPiece.pc[0] = Evaluation::GetNnuePieceNum(mPieceType, player);
            nnue_state[ply + 1].dirtyPiece.from[0] = mFrom;
            nnue_state[ply + 1].dirtyPiece.to[0] = mTo;
        }

        if (!board.MakeMove(m, true)) {
            continue;
        }

        movesSearched++;

        if (!bestMove.isValid())
            bestMove = m;

        uint64_t hash_after_move = board.getHash();
        bool irreversible = (m.getPieceType() == PAWN) || (isCapture);
        repetitionTable.Push(hash_after_move, irreversible);

        int score = -negamax(depth - 1, -beta, -alpha, ply + 1);

        repetitionTable.TryPop();
        board.UndoMove(m, true);
        if (stop) return alpha;

		if (score >= beta) {
            return beta;
        }

        if (score > alpha) {
            alpha = score;
            bestMove = m;
        }
    }

    if (movesSearched == 0) {
        return inCheck ? -MATE_SCORE + ply : 0;
    }

    return alpha;
}

void Searcher::PrepareSearcher() {
    startTime = now_ms();
    stop = false;
    isStoppedManually = false;
    isSearching = true;
    nodes = 0;

    if (rtum == RobotTimeUsageMode::FIXED_TIME) {
        softTimeLimit = fixedTimePerMoveMs;
        hardTimeLimit = fixedTimePerMoveMs;
    }
    else {
        int movesToGo = 40;
        softTimeLimit = timeLeftMs / movesToGo;
        softTimeLimit += (incrementMs * 8) / 10;

        if (softTimeLimit >= timeLeftMs) {
            softTimeLimit = std::max((long long)100, timeLeftMs - 200);
        }
        if (softTimeLimit < 100) {
            softTimeLimit = 100;
        }

        hardTimeLimit = softTimeLimit * 3;
        if (hardTimeLimit >= timeLeftMs - 100) {
            if (timeLeftMs < 1000) {
                hardTimeLimit = timeLeftMs / 2;
            }
            else {
                hardTimeLimit = timeLeftMs - 300;
            }
        }
    }

    repetitionTable.Init(board);
    repetitionTable.Push(board.getHash(), false);

    nnue_state[0].dirtyPiece.dirtyNum = 0;
    nnue_state[0].accumulator.computedAccumulation = 0;
}

Move Searcher::IterativeDeepening() {
    PrepareSearcher();

    MoveList rootMoves;
    MoveGenerator::GenerateMoves(board, rootMoves);

    std::vector<Move> legalRootMoves;
    for (const Move& m : rootMoves) {
        if (board.MakeMove(m, true)) {
            legalRootMoves.push_back(m);
            board.UndoMove(m, true);
        }
    }

    if (legalRootMoves.empty()) {
        isSearching = false;
        return Move();
    }

    if (legalRootMoves.size() == 1) {
        isSearching = false;
        return legalRootMoves[0];
    }

    Move bestMoveOverall = legalRootMoves[0];
    Move previousBestMove = Move();
    int bestScoreOverall = 0;
    int lastScore = 0;
    int stableBestMoveCount = 0;

    for (int depth = 1; depth <= currentSettings.maxDepth; depth++) {
        int alpha = -MATE_SCORE;
        int beta = MATE_SCORE;

        Move bestMoveThisDepth = Move();
        int bestScoreThisDepth = -MATE_SCORE;

        for (const Move& m : legalRootMoves) {
            board.MakeMove(m, true);

            int score = -negamax(depth - 1, -beta, -alpha, 1);

            board.UndoMove(m, true);

            if (stop) break;

            if (score > bestScoreThisDepth) {
                bestScoreThisDepth = score;
                bestMoveThisDepth = m;
            }
            if (score > alpha) {
                alpha = score;
            }
        }

        if (stop) break;

        bool inCrisis = (depth > 3 && bestScoreThisDepth < lastScore - 50);

        if (bestMoveOverall == previousBestMove) {
            stableBestMoveCount++;
        }
        else {
            stableBestMoveCount = 0;
            previousBestMove = bestMoveThisDepth;
        }

        long long timeSpent = now_ms() - startTime;

        bestMoveOverall = bestMoveThisDepth;
        bestScoreOverall = bestScoreThisDepth;
        lastScore = bestScoreThisDepth;

        if (rtum == RobotTimeUsageMode::FIXED_TIME) {
            if (timeSpent >= fixedTimePerMoveMs) {
                break;
            }
        }
        else {
            if (!inCrisis && stableBestMoveCount >= 3 && timeSpent >= (softTimeLimit * 0.6)) break;
            if (timeSpent >= softTimeLimit && !inCrisis) break;
            if (timeSpent * 2.5 > hardTimeLimit) break;
        }

        LOG_DEBUG("info depth " << depth << " score "
            << ((abs(bestScoreThisDepth) > MATE_SCORE_BOUND)
                ? "mate " + std::to_string((bestScoreThisDepth > 0) ? (MATE_SCORE + 1 - bestScoreThisDepth) / 2 : -(MATE_SCORE + 1 + bestScoreThisDepth) / 2)
                : "cp " + std::to_string(board.getSideToMove() == WHITE ? bestScoreThisDepth : -bestScoreThisDepth))
            << " time " << (now_ms() - startTime)
            << " nodes " << nodes
            << " pv " << bestMove.toAlgebraic());

        if (IsMateScore(bestScoreThisDepth)) break;
    }

    LOG_DEBUG("Final Score: "
        << ((abs(lastScore) > MATE_SCORE_BOUND)
            ? "mate " + std::to_string((lastScore > 0) ? (MATE_SCORE + 1 - lastScore) / 2 : -(MATE_SCORE + 1 + lastScore) / 2)
            : "cp " + std::to_string(board.getSideToMove() == WHITE ? lastScore : -lastScore)));

    isSearching = false;

    if (isStoppedManually) return Move();

    return bestMoveOverall;
}

Move Searcher::GetRobotMove() {
    return IterativeDeepening();
}

void Searcher::ClearSearcher() {
    repetitionTable.Clear();
    movesWithoutBlunderOnPropuse = 0;
}

void Searcher::setDifficulty(const Difficulty& diff) {

    currentDiff = diff;

    switch (diff) {
    case Difficulty::EASY:
        currentSettings = SearcherSettings::getSettings(Difficulty::EASY);
        break;

    case Difficulty::MEDIUM:
        currentSettings = SearcherSettings::getSettings(Difficulty::MEDIUM);
        break;

    case Difficulty::HARD:
        currentSettings = SearcherSettings::getSettings(Difficulty::HARD);
        break;

    case Difficulty::IMPOSSIBLE:
        currentSettings = SearcherSettings::getSettings(Difficulty::IMPOSSIBLE);
        currentSettings.maxDepth = MAXIMUM_DEPTH;
        break;

    default:
        break;
    }
}

std::string Searcher::getName() const {
    return "Régi robot";
}

std::string Searcher::getNameToSaveInFile() const {
    return "Old_searcher";
}

Difficulty Searcher::getDifficulty() const {
    return currentDiff;
}

std::string Searcher::getDifficultyString() const {
    switch (currentDiff) {
    case Difficulty::EASY:
        return "Kezdő";

    case Difficulty::MEDIUM:
        return "Haladó";

    case Difficulty::HARD:
        return "Nehéz";

    case Difficulty::IMPOSSIBLE:
        return "Mester";

    default:
        break;
    }

    return "";
}

std::string Searcher::getBotDirectoryPath() const {
    return "bots\\oldSearcher";
};
