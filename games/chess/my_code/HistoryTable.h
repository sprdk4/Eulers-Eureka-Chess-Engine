//
// Created by Shawn Roach on 4/6/2017.
//

#ifndef CPP_CLIENT_HISTORYTABLE_H
#define CPP_CLIENT_HISTORYTABLE_H

#include "Move.h"
#include "MoveSelection.h"

namespace BitBoard {

    class HistoryTable {
    public:
        //uint32_t historyTable[2][6][64][64];//[side][piece][from][to]
        uint32_t historyTable[2][64][64];//[side][from][to]
        //uint32_t historyTable[2][6][64];//[side][piece][to]

        uint32_t& fetchHistory(const PackedMove &move);

        void addHistoryToMove(PackedMove &move);

        void updateHistory(const PackedMove &packedMove, depth_t depth);

        void zeroTable();
    };


    static HistoryTable history;
}


#endif //CPP_CLIENT_HISTORYTABLE_H
