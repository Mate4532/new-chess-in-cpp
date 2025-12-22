#include "RepetitionTable.h"
#include "Board.h"

void RepetitionTable::Init(const Board& board) {
    count = 0;

    for (int i = 0; i <= board.getPly(); i++) {
        uint64_t hash = board.getHash(i);
        bool reset = (board.getHalfMoveClock(i) == 0);
        Push(hash, reset);
    }
}

void RepetitionTable::Push(uint64_t hash, bool reset)
{
    if (count < MAX_REPETITION) {
        hashes[count] = hash;
        startIndices[count + 1] = reset ? count : startIndices[count];
    }
    ++count;
}

void RepetitionTable::TryPop()
{
    if (count > 0)
        --count;
}

bool RepetitionTable::Contains(uint64_t hash) const {
    if (count < 2) return false;

    int start = startIndices[count - 1];

    for (int i = start; i < count - 1; i++) {
        if (hashes[i] == hash) {
            return true;
        }
    }
    return false;
}
