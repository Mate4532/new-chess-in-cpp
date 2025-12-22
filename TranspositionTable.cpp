#include "TranspositionTable.h"

TranspositionTable::TranspositionTable(size_t mb) {
    size_t entryCount = (mb * 1024 * 1024) / sizeof(TTEntry);
    table.resize(entryCount);
    Clear();
}

void TranspositionTable::Store(uint64_t hash, int score, int depth, TTFlag flag, Move bestMove) {
    size_t index = (hash ^ (hash >> 32)) % table.size();
    TTEntry& e = table[index];

    if (e.hash == 0 ||
        e.depth < depth ||
        (e.depth == depth && flag == EXACT)) {
        e.hash = hash;
        e.score = score;
        e.depth = depth;
        e.flag = flag;
        e.bestMove = bestMove;
    }
}

bool TranspositionTable::Probe(uint64_t hash, int depth, int alpha, int beta, int& score, Move& bestMove) {
    size_t index = (hash ^ (hash >> 32)) % table.size();
    TTEntry& e = table[index];

    if (e.hash != hash)
        return false;

    bestMove = e.bestMove;

    if (e.depth < depth)
        return false;

    if (e.flag == EXACT) {
        score = e.score;
        return true;
    }

    if (e.flag == ALPHA && e.score <= alpha) {
        score = e.score;
        return true;
    }

    if (e.flag == BETA && e.score >= beta) {
        score = e.score;
        return true;
    }

    return false;
}

void TranspositionTable::Clear() {
    for (auto& e : table) {
        e.hash = 0;
        e.depth = 0;
        e.flag = EXACT;
        e.score = 0;
        e.bestMove = Move();
    }
}
