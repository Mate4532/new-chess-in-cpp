#include "RepetitionTable.h"


void RepetitionTable::Init(const Board& board) {
    count = 0;
	const std::vector<uint64_t>& repetitionHistory = board.getRepetitionHistory();
	count = repetitionHistory.size();

    for (int i = 0; i < repetitionHistory.size(); ++i) {
		hashes[i] = repetitionHistory[i];
        startIndices[i] = 0;
    }
	startIndices[repetitionHistory.size()] = 0;
}

void RepetitionTable::Push(uint64_t hash, bool reset) {
    
    if (count < MAX_REPETITION) {
		hashes[count] = hash;
        startIndices[count + 1] = reset ? count : startIndices[count];
    }
    count++;
}

void RepetitionTable::TryPop() {
    if (count > 0) count--;
}

bool RepetitionTable::Contains(uint64_t hash) const {
    int start = startIndices[count];

    for (int i = start; i < count - 1; i++) {
        if (hashes[i] == hash) return true;
    }

    return false;
}