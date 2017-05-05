//
// Created by Shawn Roach on 3/9/2017.
//

#ifndef CPP_CLIENT_MOVE_H
#define CPP_CLIENT_MOVE_H

#include <cstdint>

namespace BitBoard {
    typedef unsigned long long int u64;

/*
packing moves into 32 bits
bits
first byte  0 - 5  : from square
            6      : set if from square had been set in notYetMoved (set during performMove)
            7      : set if to square had been set in notYetMoved (set during performMove)
second byte 8 - 13 : to square
third byte  16-18  : piece type moved
                      000=none (illegal)
                      001=king 010=pawn 011=bishop 100=knight 101=rook 110=queen
            19     : double pawn forward flag //needed to set en passante possible
                       0 for not set 1 for pawn moving twice
            20     : queen side castle
            21     : king side castle
                       bits 20-21 are 00 for not castle 10 for queen side castle, 01 for king side castle
            22     : checking flag
            23-25  : capture type (stores each type of piece:
                       bits 23-25 are 000=none 010=pawn 011=bishop 100=knight 101=rook 110=queen
            26     : en passant flag
                       0 for no en passant 1 for performs en passant
            27     : equal capture flag, set to add bias during search
            28-30  : promotion type
                       bits 28-30 are 000=none 011=bishop 100=knight 101=rook 110=queen
            31     : capture is a winning capture, set to add bias during search
            NOTE: doing something funky in movelist, may need to refer to that for additional info, until I get around to refactoring
unused bits: 14-15

            when history table is turned on, the move is 64 bits instead of 32 bits
            32-63  : history table value, set to all ones in case of promotions and captures
*/

/*
 * all my experiments against myself show that using the history table
 * doesn't yield any performance increases, and actually causes it
 * to perform marginally worse
 */
//#define useHistoryTable
    struct PackedMove {
#ifdef useHistoryTable
        typedef uint64_t packedMoveType;
        static constexpr int history_shift = 32;
        static constexpr uint32_t max_history_value= ~0u;
#else
        typedef uint32_t packedMoveType;
#endif

        static constexpr int piece_moved_shift = 15;
        static constexpr int captured_shift = 22;
        static constexpr int promotion_shift = 27;
        static constexpr int notYetMoved_from_shift = 6;
        static constexpr int notYetMoved_to_shift = 7;
        static constexpr int winning_capture_flag_shift = 31;
        static constexpr int equal_capture_flag_shift = 27;
        static constexpr int will_check_flag_shift = 22;

        static constexpr packedMoveType notYetMoved_from_mask = 0b1ull << notYetMoved_from_shift;
        static constexpr packedMoveType notYetMoved_to_mask = 0b1ull << notYetMoved_to_shift;

        static constexpr packedMoveType from_mask = 0b111111ull;
        static constexpr packedMoveType to_mask = 0b111111ull << 8;
        static constexpr packedMoveType piece_moved_mask = 0b1110ull << piece_moved_shift;
        static constexpr packedMoveType double_pawn_forward_flag = 0b1ull << 19;
        static constexpr packedMoveType castle_mask = 0b11ull << 20;
        static constexpr packedMoveType queen_castle_mask = 0b01ull << 20;
        static constexpr packedMoveType king_castle_mask = 0b10ull << 20;
        static constexpr packedMoveType captured_mask = 0b1110ull << captured_shift;
        static constexpr packedMoveType en_passant_flag = 0b1ull << 26;
        static constexpr packedMoveType equal_capture_flag = 1ull << equal_capture_flag_shift;
        static constexpr packedMoveType promotion_mask = 0b1110ull << promotion_shift;
        static constexpr packedMoveType winning_capture_flag = 1ull << winning_capture_flag_shift;
        static constexpr packedMoveType will_check_flag = 1ull << will_check_flag_shift;
        static constexpr packedMoveType remove_notYetMoved_mask = ~(notYetMoved_to_mask | notYetMoved_from_mask);

        packedMoveType m;

        operator packedMoveType &() { return m; }

        inline u64 from() const {
            return 1ull << (m & from_mask);
        }

        inline unsigned fromTableIndex() const {
            return (m & from_mask);
        }

        inline u64 to() const {
            return 1ull << ((m & to_mask) >> 8);
        }

        inline unsigned toTableIndex() const {
            return ((m & to_mask) >> 8);
        }

        inline packedMoveType didToMove() const {
            return m & notYetMoved_to_mask;
        }

        inline packedMoveType didFromMove() const {
            return m & notYetMoved_from_mask;
        }

        inline packedMoveType pieceMoved() const {
            return (m & piece_moved_mask) >> piece_moved_shift;
        }

        inline packedMoveType isDoublePawnForward() const {
            return m & double_pawn_forward_flag;
        }

        inline packedMoveType didCastle() const {
            return m & castle_mask;
        }

        inline packedMoveType didKingSideCastle() const {
            return m & king_castle_mask;
        }

        inline packedMoveType didQueenCastleMask() const {
            return m & queen_castle_mask;
        }

        inline packedMoveType didCapture() const {
            return m & captured_mask;
        }

        inline packedMoveType pieceCaptured() const {
            return (m & captured_mask) >> captured_shift;
        }

        inline packedMoveType didPromote() const {
            return m & promotion_mask;
        }

        inline packedMoveType promotionType() const {
            return (m & promotion_mask) >> promotion_shift;
        }

        inline packedMoveType didEnPassant() const {
            return m & en_passant_flag;
        }

        inline bool isNonQuiescence() const {
            return (uint32_t)m > castle_mask;
        }

        inline packedMoveType didCheck() const {
            return m & will_check_flag;
        }
    };
}
#endif //CPP_CLIENT_MOVE_H
