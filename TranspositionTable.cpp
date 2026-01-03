
#include "TranspositionTable.h"

TranspositionTable::TranspositionTable(size_t mb) {

    size_t entryCount = (mb * 1024 * 1024) / sizeof(TTEntry);
    table.resize(entryCount);
    Clear();
}

void TranspositionTable::Store(uint64_t hash, int score, int depth, TTFlag flag, Move bestMove) {
    size_t index = (hash ^ (hash >> 32)) % table.size();
    TTEntry& e = table[index];

    if (e.key == 0 || e.key != hash || e.gen != generation || depth >= e.depth) {
        e.key = hash;
        e.score = (int32_t)score;
        e.depth = (int8_t)depth;
        e.type = (uint8_t)flag;
        e.gen = generation;

        if (bestMove.isValid()) {
            e.move = bestMove;
        }
    }
}

bool TranspositionTable::Probe(uint64_t hash, int depth, int alpha, int beta, int& score, Move& bestMove) {
    size_t index = (hash ^ (hash >> 32)) % table.size();
    TTEntry& e = table[index];

	if (e.key != hash)
        return false;

	bestMove = e.move;

    if (e.depth >= depth) {
        if (e.type == TT_EXACT) {
            score = e.score;
            return true;
        }

        if (e.type == TT_ALPHA && e.score <= alpha) {
            score = e.score;
            return true;
        }
        if (e.type == TT_BETA && e.score >= beta) {
            score = e.score;
            return true;
        }
    }

    return false;
}

void TranspositionTable::Clear() {
    for (auto& e : table) {
        e.key = 0;
        e.score = 0;
		e.move = Move();
        e.depth = 0;
        e.type = 0;
        e.gen = 0;
    }
    generation = 0;
}