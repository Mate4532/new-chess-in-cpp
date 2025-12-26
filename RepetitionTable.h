#pragma once
#include <cstdint>
#include <algorithm>
#include <vector>
#include "Board.h"

class Board;

class RepetitionTable {
private:
    static constexpr int MAX_REPETITION = 256;

    uint64_t hashes[MAX_REPETITION];
    int startIndices[MAX_REPETITION + 1];
    int count;

public:
    RepetitionTable() : count(0) {
        std::fill(hashes, hashes + MAX_REPETITION, 0ULL);
        std::fill(startIndices, startIndices + MAX_REPETITION + 1, 0);
    }

    void Init(const Board& board);
    void Push(uint64_t hash, bool reset);
    void TryPop();
    bool Contains(uint64_t hash) const;
};
