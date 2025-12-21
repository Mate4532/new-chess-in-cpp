#pragma once
#include "Board.h"
#include "MoveGenerator.h"
#include "MoveOrdering.h"
#include "TranspositionTable.h"
#include "PrecomputedEvaluationData.h"
#include <iostream>

class Searcher {
private:
    Board& m_board;
    TranspositionTable tt;

    int negamax(int depth, int alpha, int beta, int ply);
    int quiescence(int alpha, int beta);

    const int max_depth = 64;
    const int robot_thinking_time_ms = 3000;

    long long startTime = 0;
    bool stopSearch = false;

    int historyMoves[2][64][64];
    Move killerMoves[2][64];

	const int MATE_SCORE = 100000;

    void ClearHistory();
    void AgeHistory();

public:
    Searcher(Board& board) : m_board(board), tt(64) { 
        ClearHistory(); 
        PrecomputedEvaluationData::Init();
    }

    Move IterativeDeepening();
    Move GetBestMove();
};