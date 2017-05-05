//
// Created by Shawn Roach on 3/15/2017.
//

#include "TranspositionTable.h"

namespace BitBoard {


    transposition &TranspositionTable::getTransposition(const u64 zobristHash) {
        return table[zobristHash % tableLength];
    }

    bool transposition::shouldOverwrite(const transposition &t) {
        if(turnUsed!=t.turnUsed || depthLeft<t.depthLeft)
            return true;
        if(depthLeft==t.depthLeft){
            if(t.flag==EXACT)
                return true;
            if(flag == EXACT)
                return false;
            if(flag == t.flag){
                if(flag==BELOW_ALPHA)
                    return score>t.score;
                else // flag == ABOVE_BETA
                    return score<t.score;
            }
            return true;
        }
        return false;
        //return (depthLeft<=t.depthLeft) || turnUsed!=t.turnUsed;
    }
}