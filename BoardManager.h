#pragma once
#include "Attacks.h"
#include <iostream>
#include "Board.h"
#include <chrono>
#include <iomanip>
#include "Searcher.h"

class BoardManager {
private:
	Searcher searcher;
	bool is_white_robot;
	bool is_black_robot;

public:
	Board board;
	BoardManager(bool is_white_robot, bool is_black_robot)
		: board(),
		searcher(board),
		is_white_robot(is_white_robot),
		is_black_robot(is_black_robot) { Attacks::InitAll(); }

	void goPerft(int perftDepth);
	Move getBestMoveOnBoard() { return searcher.GetBestMove(); }
	void printBestMove();
	void MakeRobotMove();
	void startGameLoop();
};