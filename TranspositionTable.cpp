#include "TranspositionTable.h"

TranspositionTable::TranspositionTable(size_t mb) {

    size_t entryCount = (mb * 1024 * 1024) / sizeof(TTEntry);
    table.resize(entryCount, { 0, 0, 0, EXACT, Move() });
}

void TranspositionTable::Store(uint64_t hash, int score, int depth, TTFlag flag, Move bestMove) {
    size_t index = hash % table.size();

    if (table[index].hash == 0 || table[index].depth <= depth) {
        table[index] = { hash, score, depth, flag, bestMove };
    }
}

bool TranspositionTable::Probe(uint64_t hash, int depth, int alpha, int beta, int& score, Move& bestMove) {
    size_t index = hash % table.size();
    TTEntry& entry = table[index];

    if (entry.hash == hash) {
        bestMove = entry.bestMove;

        if (entry.depth >= depth) {
            if (entry.flag == EXACT) {
                score = entry.score;
                return true;
            }

            if (entry.flag == ALPHA && entry.score <= alpha) {
                score = alpha;
                return true;
            }

            if (entry.flag == BETA && entry.score >= beta) {
                score = beta;
                return true;
            }
        }
    }
    return false;
}

void TranspositionTable::Clear() {
    for (auto& entry : table) entry.hash = 0;
}
