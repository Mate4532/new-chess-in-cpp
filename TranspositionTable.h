#pragma once
#include <vector>
#include <cstdint>
#include "Move.h"

enum TTFlag : uint8_t { EXACT, ALPHA, BETA };

struct TTEntry {
    uint64_t hash;
    int score;
    int depth;
    TTFlag flag;
    Move bestMove;
};

class TranspositionTable {
public:
    TranspositionTable(size_t mb);

    void Store(uint64_t hash, int score, int depth, TTFlag flag, Move bestMove);

    bool Probe(uint64_t hash, int depth, int alpha, int beta, int& score, Move& bestMove);

    void Clear();

private:
    std::vector<TTEntry> table;
};