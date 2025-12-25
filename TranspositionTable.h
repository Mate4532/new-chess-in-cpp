#pragma once
#include <vector>
#include <cstdint>
#include "Move.h"

enum TTFlag : uint8_t { TT_NONE, TT_EXACT, TT_ALPHA, TT_BETA };

struct TTEntry {
    uint64_t key;
    int32_t  score;
    uint16_t moveValue;
    uint8_t  movePieceType;
    int8_t   depth;
    uint8_t  type;
    uint8_t  gen;
    uint8_t  padding[6];
};

class TranspositionTable {
public:
    TranspositionTable(size_t mb);

    void NewWrite() { generation++; }

    void Store(uint64_t hash, int score, int depth, TTFlag flag, Move bestMove);
    bool Probe(uint64_t hash, int depth, int alpha, int beta, int& score, Move& bestMove);
    void Clear();

private:
    std::vector<TTEntry> table;
    uint8_t generation = 0;
};