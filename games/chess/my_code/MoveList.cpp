//
// Created by Shawn Roach on 3/9/2017.
//

#include "MoveList.h"
#include "HistoryTable.h"

namespace BitBoard {

    PackedMove &MoveList::operator[](const int x) {
        return list[x];
    }

    int MoveList::size() {
        return length;
    }

    void MoveList::emplace_back(PackedMove move) {
        history.addHistoryToMove(move);
        list[length++] = move;
    }

    bool MoveList::empty() {
        return length == 0;
    }

    PackedMove *MoveList::end() {
        return &list[length];
    }

    PackedMove *MoveList::begin() {
        return &list[0];
    }

    void MoveList::erase(PackedMove *at) {
        while (++at != end()) {
            *(at - 1) = *at;
        }
        length--;
    }

}