//
// Created by Shawn Roach on 4/17/2017.
//

#ifndef CPP_CLIENT_OPENINGTABLE_H
#define CPP_CLIENT_OPENINGTABLE_H

#include <cstdint>
#include <unordered_map>
#include "MoveSelection.h"
#include "BitBoard.h"

namespace BitBoard {

    constexpr bool USE_OPENING_TABLE = true;

    struct OpeningTableEntry {
        uint64_t zobristHash;
        uint32_t bestMoveFound;
        depth_t depthLookedTo;
        score_t scoreComputed;
        int64_t timePredictedToSearchNextDepth;
    };

    class OpeningTable {
        std::unordered_map<uint64_t, OpeningTableEntry> table;
    public:
        OpeningTable() {}

        OpeningTable(color side);

        OpeningTableEntry operator[](const uint64_t &zobristHash);

    };

}


#endif //CPP_CLIENT_OPENINGTABLE_H
