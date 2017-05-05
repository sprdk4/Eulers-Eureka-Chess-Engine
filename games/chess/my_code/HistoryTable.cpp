//
// Created by Shawn Roach on 4/6/2017.
//

#include "HistoryTable.h"
#include "BitBoard.h"

BitBoard::HistoryTable history();

extern BitBoard::BitBoard bitboard;
namespace BitBoard {
    void HistoryTable::addHistoryToMove(PackedMove &move) {
#ifdef useHistoryTable
        if (move > PackedMove::castle_mask) {
            //either a capture or promotion
            move |= (u64) PackedMove::max_history_value << PackedMove::history_shift;
        } else {
            move |= (u64) fetchHistory(move) << PackedMove::history_shift;
        }
#endif
    }

    void HistoryTable::updateHistory(const PackedMove &move, depth_t depth) {
#ifdef useHistoryTable
        if ((uint32_t) move.m <= PackedMove::castle_mask) {
            fetchHistory(move) += depth * depth;
            //fetchHistory(move) += 1u << depth;
        }
#endif
    }

    uint32_t &HistoryTable::fetchHistory(const PackedMove &move) {
        //return historyTable[bitboard.side][move.pieceMoved()/2-1][move.fromTableIndex()][move.toTableIndex()];
        return historyTable[bitboard.side][move.fromTableIndex()][move.toTableIndex()];
        //return historyTable[bitboard.side][move.pieceMoved()/2-1][move.toTableIndex()];
    }

    void HistoryTable::zeroTable() {
#ifdef useHistoryTable
        for (int side = 0; side < 2; side++)
            for (int pieceMoved = 0; pieceMoved < 6; pieceMoved++)
                for (int from = 0; from < 64; from++)
                    for (int to = 0; to < 64; to++)
                        historyTable[side]
                        //[pieceMoved]
                        [from]
                        [to] = 0u;
#endif
    }
}