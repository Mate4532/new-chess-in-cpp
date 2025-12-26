#pragma once
#include "Board.h"
#include "MoveGenerator.h"
#include "MoveOrdering.h"
#include "TranspositionTable.h"
#include "PrecomputedEvaluationData.h"
#include <iostream>

class Searcher {
private:
    Board& board;
    TranspositionTable tt;

    int negamax(int depth, int alpha, int beta, int ply, const Move& prevMove = Move(), bool prevWasCapture = false);
    int quiescence(int alpha, int beta);

    const int max_depth = 64;
    const int robot_thinking_time_ms = 3000;

    long long startTime = 0;
    std::atomic<bool> stop;
    std::atomic<uint64_t> nodes;

	RepetitionTable repetitionTable;

    int historyMoves[2][64][64];
    Move killerMoves[MAX_PLY][2];

	const int MATE_SCORE = 100000;
    const int MATE_THRESHOLD = 90000;

    void ClearHistory();
    void AgeHistory();

public:
    Searcher(Board& board) : board(board), tt(128) { 
        ClearHistory(); 
        PrecomputedEvaluationData::Init();
    }

    Move IterativeDeepening();
    Move GetBestMove();

    inline int ScoreToTT(int score, int ply);
	inline int ScoreFromTT(int score, int ply);

    inline bool IsMateScore(int score) {
        return std::abs(score) > MATE_THRESHOLD;
	}

	void ClearKillers();
};