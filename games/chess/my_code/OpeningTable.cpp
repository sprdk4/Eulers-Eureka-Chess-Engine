//
// Created by Shawn Roach on 4/17/2017.
//

#include <fstream>
#include <iostream>
#include <limits>
#include "OpeningTable.h"

namespace BitBoard {

     OpeningTable::OpeningTable(color side) {
         if(!USE_OPENING_TABLE)
             return;
        std::string fileNameOfTable;
        if (side == white) {
            fileNameOfTable = "book/whiteOpeningTable.txt";
        } else {//side==black
            fileNameOfTable = "book/blackOpeningTable.txt";
        }



        std::ifstream reader(fileNameOfTable);

        while (reader.good()) {
            uint64_t zobristHash;
            uint32_t move;
            int depthLookedTo;
            int scoreComputed;
            int64_t timeTakenInSearch;
            reader >> zobristHash;
            reader >> move;
            reader >> depthLookedTo;
            reader >> scoreComputed;
            reader >> timeTakenInSearch;
            //std::cout << zobristHash << ' ' << move << ' ' << (int) depthLookedTo << ' '
            //          << (int) scoreComputed << ' ' << timeTakenInSearch << '\n';
            auto &lookUp = table[zobristHash];
            if(scoreComputed==stalemate)
                continue;
            if (lookUp.zobristHash != zobristHash || lookUp.depthLookedTo < depthLookedTo ||
                (lookUp.depthLookedTo == depthLookedTo && lookUp.timePredictedToSearchNextDepth > timeTakenInSearch)) {
                if (scoreComputed == -checkmate || scoreComputed >= checkmate - depthLookedTo) {
                    depthLookedTo = 100;
                    timeTakenInSearch = std::numeric_limits<int64_t>::max() / 2;
                }
                lookUp = {zobristHash, move, (depth_t) depthLookedTo, (score_t) scoreComputed, timeTakenInSearch};
            }
        }
         //todo figure out if I want to store the entries from opening table into the transposition table
        reader.close();

    }

    OpeningTableEntry OpeningTable::operator[](const uint64_t &zobristHash) {
        if(!USE_OPENING_TABLE)
            return {0, 0, 0, 0, 0};

        auto entry = table.find(zobristHash);
        if (entry != table.end()) {
            return entry->second;
        } else {
            return {0, 0, 0, 0, 0};
        }
    }
}
