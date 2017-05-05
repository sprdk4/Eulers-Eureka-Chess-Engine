//
// Created by Shawn Roach on 3/15/2017.
//

#ifndef CPP_CLIENT_TRANSPOSITIONTABLE_H
#define CPP_CLIENT_TRANSPOSITIONTABLE_H

#include "Move.h"
#include "Evaluation.h"
#include "MoveSelection.h"

namespace BitBoard {
    typedef unsigned long long int  u64;
    enum transposition_flag : int8_t {
        EXACT,
        ABOVE_BETA,
        BELOW_ALPHA
    };
    struct transposition {
        uint32_t hash;
        uint32_t bestMove; //not decalared as PackedMove, to ensure 32 bit width, turning on history table enlarges move size, with useless info
        score_t score;
        transposition_flag flag;
        depth_t depthLeft;
        uint8_t turnUsed;
        bool shouldOverwrite(const transposition& t);
    }__attribute__((packed));

    class TranspositionTable {
    public:
        //static constexpr unsigned tableLength = 120767447;
        //static constexpr unsigned tableLength = 87304351;
        static constexpr unsigned tableLength = 65536459;
        //static constexpr unsigned tableLength = 4369097;
        transposition table[tableLength];
        transposition &getTransposition(const u64 zobristHash);
    };
}


#endif //CPP_CLIENT_TRANSPOSITIONTABLE_H
