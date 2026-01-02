#include "Board.h"
#include "BoardManager.h"
#include "UCIParsing.h"

#include <sstream>

std::vector<std::string> tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream iss(input);
    std::string token;

    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}


void BoardManager::goPerft(int perftDepth) {

    std::cout << "Perft(" << perftDepth << ") inditasa..." << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    uint64_t nodes = board.PerftDivide(perftDepth);

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end - start;
    double seconds = elapsed.count();

    double nps = (seconds > 0) ? (nodes / seconds) : nodes;

    std::cout << "\n------------------------------------" << std::endl;
    std::cout << "Perft(" << perftDepth << ") eredmenye: " << nodes << " csomopont" << std::endl;
    std::cout << "Idotartam: " << std::fixed << std::setprecision(3) << seconds << " masodperc" << std::endl;
    std::cout << "Sebesseg: " << std::fixed << std::setprecision(0) << nps << " NPS (Nodes Per Second)" << std::endl;
    std::cout << "------------------------------------" << std::endl;
    board.PrintBoard();
}

void BoardManager::printBestMove() {
    Move best_move = getBestMoveOnBoard();

    std::cout << "Best move: " + square_to_coordinates[best_move.getFrom()] + square_to_coordinates[best_move.getTo()] << std::endl;
}

void BoardManager::MakeRobotMove() {
	uint64_t hash_before = board.getHash();
    std::cout << "Hash kereses elott: " << board.getHash() << std::endl;
    Move best_move = searcher.GetBestMove();

    uint64_t hash_after = board.getHash();
    std::cout << "Hash kereses utan: " << board.getHash() << std::endl;
    if (hash_before != hash_after) {
		std::cout << "BAJ VAN\n\n\n\n\n" << std::endl;
    }
    board.MakeMove(best_move);
    std::cout << "Robot lepese: " + best_move.toAlgebraic() << std::endl;
}

void BoardManager::startGameLoop() {

    std::string userInput;

    while (true) {

        if (board.IsDraw()) {

        }

        if (board.IsCheckMate()) {
            std::cout << "Sakkmat, " << (board.getSideToMove() == WHITE ? "Fekete" : "Feher") << "nyert!" << std::endl;
            break;
        }

        if ((board.getSideToMove() == WHITE && is_white_robot) || (board.getSideToMove() == BLACK && is_black_robot)) {
            MakeRobotMove();
        }

        else {

            if (board.IsDraw()) {

            }

            if (board.IsCheckMate()) {
                std::cout << "Sakkmat, " << (board.getSideToMove() == WHITE ? "Fekete" : "Feher") << "nyert!" << std::endl;
                break;
            }

        }

        if (is_white_robot && is_black_robot) {
            continue;
        }

        board.PrintBoard();

        std::cout << "Add meg a lepest (pl. e2e4): ";
        std::getline(std::cin >> std::ws, userInput);

        std::vector<std::string> trimmed = tokenize(userInput);

        if (trimmed.size() == 0) {
            continue;
        }

        if (trimmed[0] == "exit") break;
        else if (trimmed[0] == "undo") {

            int n = 0;

            if (trimmed.size() == 1) {
                n = 1;
            }
            else {
                n = trimmed[1][0] - '0';
            }
            for (int i = 0; i < n; ++i) {
                if (board.getPly() > 0) {
                    board.UndoMove(board.getLastMove());
                }
            }

            board.PrintBoard();
            continue;
        }

        Move move = UCIParsing::Parse(userInput, board);

        MoveList moves;
        MoveGenerator::GenerateMoves(board, moves);

        if (!moves.contains(move)) {
            std::cout << "A lepes nem ervenyes!" << std::endl;
            continue;
        }

        if (move.getPieceType() != PIECE_NONE) {
            if (board.MakeMove(move)) {
                std::cout << "Sikeres lepes!" << std::endl;
            }
            else {
                std::cout << "Szabalytalan lepes (sakkban maradsz)!" << std::endl;
            }
        }
        else {
            std::cout << "Ervenytelen koordinatak vagy ures mezo!" << std::endl;
        }

    }
}


