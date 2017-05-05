//
// Created by Shawn Roach on 3/9/2017.
//

#include "Move.h"

namespace BitBoard {

    /*
    inline u64 PackedMove::from() const {
        return 1ull << (m & from_mask);
    }

    inline u64 PackedMove::to() const {
        return 1ull << ((m & to_mask) >> 8);
    }

    inline PackedMove::packedMoveType PackedMove::pieceMoved() const {
        return (m & piece_moved_mask) >> 16;
    }

    inline bool PackedMove::operator<(const PackedMove &rhs) const {
        return m < rhs.m;
    }

    inline PackedMove::packedMoveType PackedMove::isDoublePawnForward() const {
        return m & double_pawn_forward_flag;
    }

    inline PackedMove::packedMoveType PackedMove::didCastle() const {
        return m & castle_mask;
    }

    inline PackedMove::packedMoveType PackedMove::didKingSideCastle() const {
        return m & king_castle_mask;
    }

    inline PackedMove::packedMoveType PackedMove::didQueenCastleMask() const {
        return m & queen_castle_mask;
    }

    inline PackedMove::packedMoveType PackedMove::didCapture() const {
        return m & captured_mask;
    }

    inline PackedMove::packedMoveType PackedMove::pieceCaptured() const {
        return m & captured_mask >> 22;
    }

    inline PackedMove::packedMoveType PackedMove::didPromote() const {
        return m & promotion_mask;
    }

    inline PackedMove::packedMoveType PackedMove::promotionType() const {
        return m & promotion_mask >> 26;
    }

    inline PackedMove::packedMoveType PackedMove::didEnPassant() const {
        return m & en_passant_flag;
    }
    */
}