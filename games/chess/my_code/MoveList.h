//
// Created by Shawn Roach on 3/9/2017.
//

#include "Move.h"

#ifndef CPP_CLIENT_MOVELIST_H
#define CPP_CLIENT_MOVELIST_H

namespace BitBoard{
    struct MoveList{
        //218 is the max number of legal moves that can be generated from a legal position reachable by the standard initial board state
        //https://www.chess.com/forum/view/fun-with-chess/what-chess-position-has-the-most-number-of-possible-moves
        int length=0;
        PackedMove list[218];

        PackedMove& operator[](const int x);

        void emplace_back(PackedMove move);

        //functions added to keep it compatible with old code that used std::vector<BitBoard::Move> rather than movelist
        //woah, did adding begin and end cause MoveList to be usable in range based for loops? thats awesome!
        PackedMove* begin();
        PackedMove* end();
        int size();
        bool empty();
        void erase(PackedMove* at);
    };
}
#endif //CPP_CLIENT_MOVELIST_H
