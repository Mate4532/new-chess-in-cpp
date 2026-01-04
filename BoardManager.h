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
	bool is_white_player;
	bool is_black_player;

public:
	Board board;
	BoardManager(bool is_white_robot, bool is_black_robot)
		: board(),
		searcher(board),
		is_white_robot(is_white_robot),
		is_black_robot(is_black_robot) { 
		Attacks::InitAll(); 
		is_white_player = !is_white_robot;
		is_black_player = !is_black_robot;
	}

	void goPerft(int perftDepth);
	Move getBestMoveOnBoard() { return searcher.GetBestMove(); }
	void printBestMove();
	void MakeRobotMove();
	bool didGameEnd();
	void startGameLoop();
};