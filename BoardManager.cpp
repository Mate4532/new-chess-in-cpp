#include "Board.h"
#include "BoardManager.h"
#include "UCIParsing.h"

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

    Move best_move = searcher.GetBestMove();

	board.MakeMove(best_move);
    std::cout << "Robot lepese: " + best_move.toAlgebraic() << std::endl;
}

void BoardManager::startGameLoop() {

	std::string userInput;

    while (true) {

        if (board.IsRepetition()) {
           //TODO
        }

        if (board.IsDraw()) {
            std::cout << "Dontetlen!" << std::endl;
            break;
        }

        if (board.IsCheckMate()) {
            std::cout << "Sakkmat, " << (board.getSideToMove() == WHITE ? "Fekete" : "Feher") << "nyert!" << std::endl;
            break;
        }

        if ((board.getSideToMove() == WHITE && is_white_robot) || (board.getSideToMove() == BLACK && is_black_robot))
            MakeRobotMove();

        else {
            if (board.IsRepetition()) {
                //TODO
            }

            if (board.IsDraw()) {
                std::cout << "Dontetlen!" << std::endl;
                break;
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
        std::cin >> userInput;

        if (userInput == "exit") break;

        Move move = UCIParsing::Parse(userInput, board);

        MoveList moves;
        MoveGenerator::GenerateMoves(board, moves);

        if (!moves.contains(move)){
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


