//
// Created by Shawn Roach on 1/26/2017.
//

#ifndef INC_2016FS_A_HW1A_SPRDK4_BITBOARD_H
#define INC_2016FS_A_HW1A_SPRDK4_BITBOARD_H

#include <iosfwd>
#include <vector>
#include "../impl/chess_fwd.hpp"
#include "Move.h"
#include "MoveList.h"
#include "Evaluation.h"

namespace BitBoard {

    typedef unsigned long long int u64;
    enum PieceListIndex : uint16_t {
        occ = 0b0000,//0,1=occ
        king = 0b0010,//2,3=king
        pawns = 0b0100,//4,5=pawn
        bishops = 0b0110,//6,7=bishop
        knights = 0b1000,//8,9=knight
        rooks = 0b1010,//10,11=rook
        queens = 0b1100,//12,13=queen
    };

    enum color : uint16_t {
        black = 0,
        white = 1
    };


    class BitBoard {
    private:
        u64 requiredBlockingRQ;
        u64 requiredBlockingBQ;
        u64 willUnblock;
        u64 willPutInCheck[5];
        u64 enpassanter;
        u64 willUnblockR;
        u64 willUnblockB;
        score_t materialBalance;
        bool hasCastled[2];

        PackedMove::packedMoveType willEnpassCauseCheck(u64 to, u64 from);

        void addMoves(uint32_t from, u64 tos, MoveList &list, uint32_t flags);

        void addCaptureMoves(uint32_t from, u64 tos, MoveList &list, uint32_t flags);

        void addPawnForwards(int goBack, u64 tos, MoveList &list, uint32_t flags);

        void addPawnCaptures(int goBack, u64 tos, MoveList &list, uint32_t flags);

        void addPawnForwardPromotions(int goBack, u64 tos, MoveList &list, uint32_t flags);

        void addPawnCapturePromotions(int goBack, u64 tos, MoveList &list, uint32_t flags);


    public:

        u64 zobristHash;

        u64 notYetMoved;
        u64 dangerZone;
        color side;

        u64 legalMovesToGetOutOfCheck;
        u64 *me;
        u64 *opp;
        u64 _[14];//pointed to by me and opp, indexed with PieceListIndex, ex: me[pawns] translates to _[side|pawns]

        BitBoard() {}

        BitBoard(const cpp_client::chess::Game &g, const cpp_client::chess::Player &player);

        void switchSides();

        void lookForCheckInducingMovements();

        void printBoard() const;

        void performMove(const cpp_client::chess::Move &move);

        void performMove(PackedMove &move);

        void undoMove(const PackedMove &move);

        MoveList getLegalMoves();

        friend score_t evaluate();

        friend score_t fast_evaluate();

    };
}
#endif //INC_2016FS_A_HW1A_SPRDK4_BITBOARDBOARD_H
