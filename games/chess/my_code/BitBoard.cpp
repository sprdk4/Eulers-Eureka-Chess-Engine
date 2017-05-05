//
// Created by Shawn Roach on 1/26/2017.
//

#include <iostream>
#include <cassert>
#include <sstream>
#include "../ai.hpp"
#include "BitBoard.h"
#include "ScopedBenchmark.h"
#include "BitMath.h"
#include "ZobristHashConstants.h"

#include <iostream>
#include <bitset>

namespace BitBoard {
    constexpr int rankDirection[2] = {-1, 1};

    score_t materialBalanceTable[14] = {
            0, 0, // 0,1 never refered to
            0, 0, // 2,3 never refered to, (king can't be captured)
            100, 100, // 4,5 (black pawn, white pawn)
            350, 350, // 6,7 (black/white bishop)
            300, 300, // 8,9 (black/white knight)
            500, 500, // 10,11 (black/white rook)
            900, 900 // 12,13 (black/white queen)
            //note: upon board creation, values of opponent pieces are set to negative
    };

    BitBoard::BitBoard(const cpp_client::chess::Game &g, const cpp_client::chess::Player &player) :
            side(player->color == "Black" ? black : white) {
        for (int x = 0; x < 14; x++)
            _[x] = 0;
        notYetMoved = 0;
        zobristHash = 0;

        me = &_[side];
        opp = &_[1 - side];

        for (const auto &p:g->pieces) {
            uint8_t col = 1;
            if (p->owner->color == "Black")
                col = 0;

            u64 index = toBitIndex(p->file, p->rank);
            _[col | occ] ^= index;
            if (p->type == "Rook") {
                _[col | rooks] ^= index;
            } else if (p->type == "Bishop") { ;
                _[col | bishops] ^= index;
            } else if (p->type == "King") {
                _[col | king] ^= index;
            } else if (p->type == "Queen") {
                _[col | queens] ^= index;
            } else if (p->type == "Pawn") {
                _[col | pawns] ^= index;
            } else if (p->type == "Knight") {
                _[col | knights] ^= index;
            } else {
                std::cerr << "failed to identify type: " << p->type << '\n';
            }
        }

        for (int _ind = 2; _ind < 14; _ind++) {
            forEachBitIndex_i(_[_ind]) {
                int n = toTableIndex(i);
                zobristHash ^= pieceZobristMasks[_ind - 2][n];
            }
        }

        std::string fen = g->fen;
        std::stringstream s(fen);
        std::string locations;
        s >> locations;//not going to use
        std::string activePlayer;
        s >> activePlayer;
        std::string legalCastles;
        s >> legalCastles;
        std::string enpassant;
        s >> enpassant;

        if (side)
            zobristHash ^= whitesTurnZobristMask;

        if (legalCastles.find('K') != std::string::npos) {
            notYetMoved |= whiteKingCastle;//can white king side castle
            zobristHash ^= castlingZobristMasks[2];
        }
        if (legalCastles.find('Q') != std::string::npos) {
            notYetMoved |= whiteQueenCastle;//can white queen side castle
            zobristHash ^= castlingZobristMasks[3];
        }
        if (legalCastles.find('k') != std::string::npos) {
            notYetMoved |= blackKingCastle;//can black king side castle
            zobristHash ^= castlingZobristMasks[0];
        }
        if (legalCastles.find('q') != std::string::npos) {
            notYetMoved |= blackQueenCastle;//can black queen side castle
            zobristHash ^= castlingZobristMasks[1];
        }

        enpassanter = 0;
        if (enpassant.find('-') == std::string::npos) {
            //en passant possible:
            std::string file = enpassant.substr(0, 1);
            int rank = (int) (enpassant[1] - '0');

            // polyglot book format (.bin) for zobrist hashing requires pawn to be in position for enpassant
            // to have the zobrist mask for enpassant applied, therefore I'm only setting enpassanter if it's possible
            u64 possible = toBitIndex(file, rank);
            if (activePlayer == "b") {
                if (((((~files[0]) & possible) << 7) & _[black | pawns]) ||
                    ((((~files[7]) & possible) << 9) & _[black | pawns])) {
                    zobristHash ^= enPassantFileZobristMasks[toFileIndex(possible)];
                    enpassanter = possible;
                }
            } else {
                if (((((~files[0]) & possible) >> 9) & _[white | pawns]) ||
                    ((((~files[7]) & possible) >> 7) & _[white | pawns])) {
                    zobristHash ^= enPassantFileZobristMasks[toFileIndex(possible)];
                    enpassanter = possible;
                }
            }
        }

        for (auto &e:{pawns, bishops, knights, rooks, queens}) {
            materialBalanceTable[(1 - side) | e] *= -1;
        }
        materialBalance = 0;
        for (int x = 4; x < 14; x++) {
            materialBalance += __builtin_popcountll(_[x]) * materialBalanceTable[x];
        }

        hasCastled[0] = false;
        hasCastled[1] = false;
    }

    char toPieceAt(const u64 *const list, const u64 index) {
        if (list[pawns] & index)
            return 'p';
        if (list[knights] & index)
            return 'n';
        if (list[king] == index)
            return 'k';
        if (list[queens] & index)
            return 'q';
        if (list[bishops] & index)
            return 'b';
        if (list[rooks] & index)
            return 'r';
        return '?';
    }


    void BitBoard::printBoard() const {
        unsigned count = 0;
        std::cout << "   ";
        for (unsigned x = 0; x < 8; x++)
            printf("%*c", -2, x + 'a');
        std::cout << '\n';
        for (int x = 8 - 1; x >= 0; x--) {
            printf("%*u ", 2, 8 - count++);
            for (int q = 0; q < 8; q++) {
                u64 index = 1ull << (x * 8 + q);
                if (_[white | occ] & index) {
                    std::cout << (char) toupper(toPieceAt(&_[white], index));
                } else if (_[black | occ] & index) {
                    std::cout << (toPieceAt(&_[black], index));
                } else {
                    std::cout << '.';
                }
                std::cout << ' ';
            }
            std::cout << '\n';
        }
    }

    inline u64 doLShift(u64 val, int shift) {
        return (shift > 0 ? ((val) << (shift)) : ((val) >> (-shift)));
    }

    /*
  northwest    north   northeast
  noWe         nort         noEa
          +7    +8    +9
              \  |  /
  west    -1 <-  0 -> +1    east
              /  |  \
          -9    -8    -7
  soWe         sout         soEa
  southwest    south   southeast
    */

    void BitBoard::addMoves(uint32_t from, u64 tos, MoveList &list, uint32_t flags) {
        u64 capturingTos = tos & _[(1 - side) | occ];
        addCaptureMoves(from, capturingTos, list, flags);
        tos ^= capturingTos;
        while (tos) {
            uint32_t to = toTableIndex(tos);
            list.emplace_back({from | (to << 8) | flags});
            tos &= tos - 1;
        }
    }

    void BitBoard::addCaptureMoves(uint32_t from, u64 tos, MoveList &list, uint32_t flags) {
        uint32_t pieceMoved = (flags & PackedMove::piece_moved_mask) >> PackedMove::piece_moved_shift;
        while (tos) {
            uint32_t to = toTableIndex(tos);
            u64 toind = 1ull << to;
            uint32_t capturedPiece;
            for (capturedPiece = pawns; !(toind & opp[capturedPiece]); capturedPiece += 2) {/*empty loop*/}

            if (pieceMoved <= capturedPiece) {
                if (pieceMoved < capturedPiece) {
                    list.emplace_back({from | (to << 8) | flags | (capturedPiece << PackedMove::captured_shift)
                                       | PackedMove::winning_capture_flag});
                } else {
                    list.emplace_back({from | (to << 8) | flags | (capturedPiece << PackedMove::captured_shift)
                                       | PackedMove::equal_capture_flag});
                }

            } else {
                list.emplace_back({from | (to << 8) | flags | (capturedPiece << PackedMove::captured_shift)});
            }
            tos &= tos - 1;
        }
    }

    void BitBoard::addPawnForwards(int goBack, u64 tos, MoveList &list, uint32_t flags) {
        while (tos) {
            uint32_t to = toTableIndex(tos);
            uint32_t from = ((uint32_t) (to - goBack)) % 64;
            if ((((1ull << from) & willUnblockR) && ((1ull << to) & ~willUnblockR)) ||
                (((1ull << from) & willUnblockB) && ((1ull << to) & ~willUnblockB))) {
                list.emplace_back({from | (to << 8) | flags | PackedMove::will_check_flag});
            } else {
                list.emplace_back({from | (to << 8) | flags});
            }
            tos &= tos - 1;
        }
    }

    void BitBoard::addPawnCaptures(int goBack, u64 tos, MoveList &list, uint32_t flags) {
        while (tos) {
            uint32_t to = toTableIndex(tos);
            u64 toind = 1ull << to;
            uint32_t from = ((uint32_t) (to - goBack)) % 64;
            uint32_t capturedPiece;

            for (capturedPiece = pawns; !(toind & opp[capturedPiece]); capturedPiece += 2) {/*empty loop*/}

            if (capturedPiece > pawns)
                from |= PackedMove::winning_capture_flag;
            else
                from |= PackedMove::equal_capture_flag;

            if (1ull << from & willUnblock) {
                list.emplace_back({from | (to << 8) | flags | (capturedPiece << PackedMove::captured_shift)
                                   | PackedMove::will_check_flag});
            } else {
                list.emplace_back({from | (to << 8) | flags | (capturedPiece << PackedMove::captured_shift)});
            }

            tos &= tos - 1;
        }
    }

    void BitBoard::addPawnForwardPromotions(int goBack, u64 tos, MoveList &list, uint32_t flags) {
        while (tos) {
            uint32_t to = toTableIndex(tos);
            uint32_t from = ((uint32_t) (to - goBack)) % 64;
            uint32_t move = from | (to << 8) | flags;

            //not including bishop and rook promotions
            if ((1ull << from) & willUnblock) {
                move |= PackedMove::will_check_flag;
                list.emplace_back({move | (queens << PackedMove::promotion_shift)});
                list.emplace_back({move | (knights << PackedMove::promotion_shift)});
            } else {
                u64 totalOccWithoutPawn = (_[white | occ] | _[black | occ]) ^(1ull << from);
                if ((getLegalRookAttacks(totalOccWithoutPawn, 1ull << to) & opp[king]) ||
                    (getLegalBishopAttacks(totalOccWithoutPawn, 1ull << to) & opp[king])) {
                    list.emplace_back({move | (queens << PackedMove::promotion_shift) | PackedMove::will_check_flag});
                } else {
                    list.emplace_back({move | (queens << PackedMove::promotion_shift)});
                }
                if (willPutInCheck[knights / 2 - 2] & 1ull << to) {
                    list.emplace_back({move | (knights << PackedMove::promotion_shift) | PackedMove::will_check_flag});
                } else {
                    list.emplace_back({move | (knights << PackedMove::promotion_shift)});
                }
            }
            tos &= tos - 1;
        }
    }

    void BitBoard::addPawnCapturePromotions(int goBack, u64 tos, MoveList &list, uint32_t flags) {
        while (tos) {
            uint32_t to = toTableIndex(tos);
            u64 toind = 1ull << to;
            uint32_t from = ((uint32_t) (to - goBack)) % 64;
            uint32_t capturedPiece;
            for (capturedPiece = pawns; !(toind & opp[capturedPiece]); capturedPiece += 2) {/*empty loop*/}
            uint32_t move = from | (to << 8) | flags | (capturedPiece << PackedMove::captured_shift);

            //not including bishop and rook promotions
            if ((1ull << from) & willUnblock) {
                move |= PackedMove::will_check_flag;
                list.emplace_back({move | (queens << PackedMove::promotion_shift)});
                list.emplace_back({move | (knights << PackedMove::promotion_shift)});
            } else {
                u64 totalOccWithoutPawn = (_[white | occ] | _[black | occ]) ^(1ull << from);
                if ((getLegalRookAttacks(totalOccWithoutPawn, toind) & opp[king]) ||
                    (getLegalBishopAttacks(totalOccWithoutPawn, toind) & opp[king])) {
                    list.emplace_back({move | (queens << PackedMove::promotion_shift) | PackedMove::will_check_flag});
                } else {
                    list.emplace_back({move | (queens << PackedMove::promotion_shift)});
                }
                if (willPutInCheck[knights / 2 - 2] & toind) {
                    list.emplace_back({move | (knights << PackedMove::promotion_shift) | PackedMove::will_check_flag});
                } else {
                    list.emplace_back({move | (knights << PackedMove::promotion_shift)});
                }
            }
            tos &= tos - 1;
        }
    }

    MoveList BitBoard::getLegalMoves() {
        me = &_[side];
        opp = &_[1 - side];

        lookForCheckInducingMovements();
        MoveList list;

        u64 totalOcc = _[white | occ] | _[black | occ];

        //king moves
        {
            u64 legalMovesK = kingAttacks[toTableIndex(me[king])] & ~me[occ] & ~dangerZone;
            if (me[king] & willUnblock) {
                addMoves(toTableIndex(me[king]),//from
                         legalMovesK & ~willUnblock,//will unblock piece to cause check
                         list,
                         king << PackedMove::piece_moved_shift | PackedMove::will_check_flag);
                addMoves(toTableIndex(me[king]),//from
                         legalMovesK & willUnblock,//will continue to block piece from causing check
                         list,
                         king << PackedMove::piece_moved_shift);
            } else {
                addMoves(toTableIndex(me[king]),//from
                         legalMovesK,//legal moves to
                         list,
                         king << PackedMove::piece_moved_shift);
            }
        }

        if (!legalMovesToGetOutOfCheck) {
            // there does not exist a move to stop check by any piece other than king
            // this is caused by multiple pieces putting king in check at once
            // thus I don't need to do the rest of move generation past the king
            return list;
        }

        //bishop moves
        forEachBitIndex_i(me[bishops] & ~requiredBlockingRQ) {//check if needs to block a rook slider, if so bishops can't make a move
            //get legal moves, then remove all of my pieces from that
            u64 legalMovesB = getLegalBishopAttacks(totalOcc, i);
            legalMovesB &= ~me[occ];//filter out moves that would be capturing my own pieces
            legalMovesB &= legalMovesToGetOutOfCheck;//filter out moves that don't fix check
            //if this piece must block, filter moves that are on the blocking line
            if (i & requiredBlockingBQ)
                legalMovesB &= requiredBlockingBQ;

            if (i & willUnblock) {
                addMoves(toTableIndex(i), legalMovesB, list, bishops << PackedMove::piece_moved_shift | PackedMove::will_check_flag);
            } else {
                addMoves(toTableIndex(i), legalMovesB & willPutInCheck[bishops / 2 - 2], list, bishops << PackedMove::piece_moved_shift
                                                                                               | PackedMove::will_check_flag);
                addMoves(toTableIndex(i), legalMovesB & ~willPutInCheck[bishops / 2 - 2], list, bishops << PackedMove::piece_moved_shift);
            }
        }

        //rook moves
        forEachBitIndex_i(me[rooks] & ~requiredBlockingBQ) {//check if needs to block a bishop slider, if so rooks can't make a move
            //get legal moves, then remove all of my pieces from that
            u64 legalMovesR = getLegalRookAttacks(totalOcc, i);
            legalMovesR &= ~me[occ];//filter out moves that would be capturing my own pieces
            legalMovesR &= legalMovesToGetOutOfCheck;//filter out moves that don't fix check
            //if this piece must block, filter moves that are on the blocking line
            if (i & requiredBlockingRQ)
                legalMovesR &= requiredBlockingRQ;

            if (i & willUnblock) {
                addMoves(toTableIndex(i), legalMovesR, list, rooks << PackedMove::piece_moved_shift | PackedMove::will_check_flag);
            } else {
                addMoves(toTableIndex(i), legalMovesR & willPutInCheck[rooks / 2 - 2], list, rooks << PackedMove::piece_moved_shift
                                                                                             | PackedMove::will_check_flag);
                addMoves(toTableIndex(i), legalMovesR & ~willPutInCheck[rooks / 2 - 2], list, rooks << PackedMove::piece_moved_shift);
            }
        }

        //queens moves
        forEachBitIndex_i(me[queens]) {
            //check rook sliding moves first
            //cant do rook slidings, if must block bishop slider
            u64 legalMoves = 0;
            if (!(i & requiredBlockingBQ)) {
                //get legal moves, then remove all of my pieces from that
                u64 legalMovesR = getLegalRookAttacks(totalOcc, i);
                legalMovesR &= ~me[occ];//filter out moves that would be capturing my own pieces
                legalMovesR &= legalMovesToGetOutOfCheck;//filter out moves that don't fix check
                //if this piece must block, filter moves that are on the blocking line
                if (i & requiredBlockingRQ)
                    legalMovesR &= requiredBlockingRQ;
                legalMoves = legalMovesR;
            }

            //cant do bishop sliding if must block rook slider
            if (!(i & requiredBlockingRQ)) {
                u64 legalMovesB = getLegalBishopAttacks(totalOcc, i);
                legalMovesB &= ~me[occ];//filter out moves that would be capturing my own pieces
                legalMovesB &= legalMovesToGetOutOfCheck;//filter out moves that don't fix check
                //if this piece must block, filter moves that are on the blocking line
                if (i & requiredBlockingBQ)
                    legalMovesB &= requiredBlockingBQ;
                legalMoves |= legalMovesB;
            }
            addMoves(toTableIndex(i), legalMoves & willPutInCheck[queens / 2 - 2], list, queens << PackedMove::piece_moved_shift
                                                                                         | PackedMove::will_check_flag);
            addMoves(toTableIndex(i), legalMoves & ~willPutInCheck[queens / 2 - 2], list, queens << PackedMove::piece_moved_shift);

        }

        //knight moves
        forEachBitIndex_i(me[knights] & ~(requiredBlockingBQ | requiredBlockingRQ)) {//check if needs to block, if so knights can't  move
            //get legal moves, then remove all of my pieces from that
            u64 legalMovesN = knightAttacks[toTableIndex(i)];
            legalMovesN &= ~me[occ];//filter out moves that would be capturing my own pieces
            legalMovesN &= legalMovesToGetOutOfCheck;//filter out moves that don't fix check

            if (i & willUnblock) {
                addMoves(toTableIndex(i), legalMovesN, list, knights << PackedMove::piece_moved_shift | PackedMove::will_check_flag);
            } else {
                addMoves(toTableIndex(i), legalMovesN & willPutInCheck[knights / 2 - 2], list, knights << PackedMove::piece_moved_shift
                                                                                               | PackedMove::will_check_flag);
                addMoves(toTableIndex(i), legalMovesN & ~willPutInCheck[knights / 2 - 2], list, knights << PackedMove::piece_moved_shift);
            }

        }

        //pawn moves
        if (me[pawns]) {//early quit of checking in case there are no pawns
            u64 forward = me[pawns] & ~requiredBlockingBQ;//filter out pawns that are blocking bishop sliders

            constexpr int forwardShift[2] = {64 - 8, 8};//index 0 for black, index 1 for white
            int shift = forwardShift[side];
            u64 moveWontBlock =
                    (rotateLeft(forward & requiredBlockingRQ, shift)) & ~requiredBlockingRQ; //moves that cause a pawn to unblock
            forward = rotateLeft(forward, shift);//move pawns forward
            forward ^= moveWontBlock; //filter out unblocking moves
            forward &= (~totalOcc);//not allowed to capture
            u64 singleforward = forward & legalMovesToGetOutOfCheck;//filter out moves that don't get out of check

            //pawn forward moves
            u64 tos = singleforward & (~(ranks[0] | ranks[7]));

            addPawnForwards(shift,
                            tos & willPutInCheck[pawns / 2 - 2], //can't be a promotional move
                            list,
                            (pawns << PackedMove::piece_moved_shift) | PackedMove::will_check_flag);
            addPawnForwards(shift,
                            tos & ~willPutInCheck[pawns / 2 - 2], //can't be a promotional move
                            list,
                            (pawns << PackedMove::piece_moved_shift));


            //pawn promotional forwards
            addPawnForwardPromotions(shift,
                                     singleforward & ((ranks[0] | ranks[7])), //can't be a promotional move
                                     list,
                                     (pawns << PackedMove::piece_moved_shift));

            constexpr u64 duoforwardLandsOnRank[2] = {ranks[4], ranks[3]};//index 0 for black, index 1 for white
            forward = rotateLeft(forward, shift) //move forward once again for double forward pawns
                      & legalMovesToGetOutOfCheck//move pawns forward and filter non check fixing moves
                      & (duoforwardLandsOnRank[side])//filter out pawns that had already made a move;
                      & ~totalOcc; //duo forward not allowed to capture
            //it has already been determine if the pawn is being pinned, no need to check again for duo forward, as it will be the same
            shift *= 2;
            addPawnForwards(shift,
                            forward & willPutInCheck[pawns / 2 - 2],
                            list,
                            (pawns << PackedMove::piece_moved_shift) | PackedMove::double_pawn_forward_flag
                            | PackedMove::will_check_flag);
            addPawnForwards(shift,
                            forward & ~willPutInCheck[pawns / 2 - 2],
                            list,
                            (pawns << PackedMove::piece_moved_shift) | PackedMove::double_pawn_forward_flag);

            constexpr int leftRightShifts[2][2] = {{64 - 9, 64 - 7},
                                                   {7,      9}};
            constexpr u64 fileMasks[2] = {~files[0], ~files[7]};
            for (int lr = 0; lr < 2; lr++) {
                shift = leftRightShifts[side][lr];
                u64 diag = me[pawns] & ~requiredBlockingRQ & fileMasks[lr]; //filter out pawns blocking rook sliders and warping moves

                moveWontBlock = (rotateLeft(diag & requiredBlockingBQ, shift)) & ~requiredBlockingBQ;//moves causing pawn to unblock
                diag = rotateLeft(diag, shift); //move pawns diagonaly
                diag ^= moveWontBlock;//filter out unblocking moves

                u64 enpass = enpassanter & diag;
                if (enpass) {
                    if (enpass & (legalMovesToGetOutOfCheck | (rotateLeft(legalMovesToGetOutOfCheck & opp[pawns], forwardShift[side])))) {
                        //en passant either fixes check as any other piece would, or is taking a pawn that is putting in check
                        if ((me[king] & ((side) ? (ranks[4]) : (ranks[3]))) &&
                            //king is on same rank as pawn doing enpassant, and pawn that will be captured
                            (((side) ? (ranks[4]) : (ranks[3])) & (opp[queens] | opp[rooks]))) {//queen or rook on same rank too
                            if (!(getLegalRookAttacks(
                                    totalOcc ^ ((rotateRight(enpass, shift)) | (rotateRight(enpass, forwardShift[side]))), me[king]) &
                                  (opp[queens] | opp[rooks]))) {
                                //by performing the enpassant and removing the two pawns in the rank, I am not opening up the king for check
                                uint32_t to = toTableIndex(enpass);
                                uint32_t from = ((uint32_t) (to - shift)) % 64;
                                list.emplace_back({from | (to << 8)
                                                   | (pawns << PackedMove::piece_moved_shift)
                                                   | PackedMove::en_passant_flag
                                                   | willEnpassCauseCheck(enpass, 1ull << from)});
                            }
                        } else {
                            uint32_t to = toTableIndex(enpass);
                            uint32_t from = ((uint32_t) (to - shift)) % 64;
                            list.emplace_back({from | (to << 8)
                                               | (pawns << PackedMove::piece_moved_shift)
                                               | PackedMove::en_passant_flag
                                               | willEnpassCauseCheck(enpass, 1ull << from)});
                        }
                    }
                }

                diag &= legalMovesToGetOutOfCheck;//filter out moves that keep me in check

                tos = diag & (~(ranks[0] | ranks[7])) & opp[occ];
                if (tos) {
                    addPawnCaptures(shift,
                                    tos & willPutInCheck[pawns / 2 - 2],
                                    list,
                                    (pawns << PackedMove::piece_moved_shift) | PackedMove::will_check_flag);
                    addPawnCaptures(shift,
                                    tos & ~willPutInCheck[pawns / 2 - 2],
                                    list,
                                    (pawns << PackedMove::piece_moved_shift));
                }

                addPawnCapturePromotions(shift,
                                         diag & ((ranks[0] | ranks[7])) & opp[occ],
                                         list,
                                         (pawns << PackedMove::piece_moved_shift) | PackedMove::winning_capture_flag);
            };


        }

        //look for castles
        if (!(~legalMovesToGetOutOfCheck)) { //not in check
            const castleMasks *thisSidesCastleMasks = CastleMasks[side];
            for (int kq = 0; kq < 2; kq++) {
                const castleMasks &c = thisSidesCastleMasks[kq];
                if ((notYetMoved & c.castle) == c.castle && // rook/king havn't moved
                    !(c.empties & totalOcc) && // nothing in between rook and king
                    !(c.castlemask & dangerZone)) {//kings movement won't put him in check
                    //rook/king havn't moved for to allow castle, and no pieces in between them, and king moves aren't in dangerzone
                    list.emplace_back({toTableIndex(me[king])
                                       | (toTableIndex(c.kingLanding) << 8)
                                       | (king << PackedMove::piece_moved_shift)
                                       | (PackedMove::queen_castle_mask << (1 - kq))
                                       | ((getLegalRookAttacks(totalOcc ^ me[king], c.rookLanding) & opp[king]) ?
                                          PackedMove::will_check_flag : 0)});
                }
            }
        }

        return list;
    }

    void BitBoard::lookForCheckInducingMovements() {
        u64 totalOcc = _[white | occ] | _[black | occ];

        const u64 *const me = &_[side];
        const u64 *const opp = &_[1 - side];

        u64 firstPassR = getLegalRookAttacks(totalOcc, me[king]);
        u64 firstPassB = getLegalBishopAttacks(totalOcc, me[king]);

        //remove own pieces from firstPasses, as they block queen moves out from the king
        u64 occMinusOwnPiecesBlockingKingR = (me[occ] & firstPassR) ^totalOcc;
        u64 occMinusOwnPiecesBlockingKingB = (me[occ] & firstPassB) ^totalOcc;

        u64 opponentRookSlidersThatRequireBlocking = 0;
        u64 opponentBishopSlidersThatRequireBlocking = 0;
        if (occMinusOwnPiecesBlockingKingR != firstPassR) {
            // get legal rook moves going out from king, with own pieces that were blocking
            // as discovered from firstPass removed, to allow finding pieces behind them
            u64 secondPassR = getLegalRookAttacks(occMinusOwnPiecesBlockingKingR, me[king]);

            //remove opponent pieces discovered in second pass that were already disovered in first pass.
            // ie. remove opponent pieces that didn't have anything in between them and the king
            secondPassR ^= opp[occ] & firstPassR;

            opponentRookSlidersThatRequireBlocking = ((opp[rooks] | opp[queens]) & secondPassR);

            requiredBlockingRQ = 0;//reset required blocking
            forEachBitIndex_i(opponentRookSlidersThatRequireBlocking) {
                requiredBlockingRQ |= (getLegalRookAttacks(occMinusOwnPiecesBlockingKingR, i) & secondPassR) | i;
            }
        }
        if (occMinusOwnPiecesBlockingKingB != firstPassB) {
            // get legal bishop moves going out from king, with own pieces that were blocking
            // as discovered from firstPass removed, to allow finding pieces behind them
            u64 secondPassB = getLegalBishopAttacks(occMinusOwnPiecesBlockingKingB, me[king]);

            //remove opponent pieces discovered in second pass that were already disovered in first pass.
            // ie. remove opponent pieces that didn't have anything in between them and the king
            secondPassB ^= opp[occ] & firstPassB;

            opponentBishopSlidersThatRequireBlocking = ((opp[bishops] | opp[queens]) & secondPassB);

            requiredBlockingBQ = 0;//reset required blocking
            forEachBitIndex_i(opponentBishopSlidersThatRequireBlocking) {
                requiredBlockingBQ |= (getLegalBishopAttacks(occMinusOwnPiecesBlockingKingB, i) & secondPassB) | i;
            }
        }

        dangerZone = 0ull;
        u64 removedKingOcc = totalOcc ^me[king];
        forEachBitIndex_i(opp[bishops] & ~opponentBishopSlidersThatRequireBlocking) {
            dangerZone |= getLegalBishopAttacks(removedKingOcc, i);
        }
        forEachBitIndex_i(opp[rooks] & ~opponentRookSlidersThatRequireBlocking) {
            dangerZone |= getLegalRookAttacks(removedKingOcc, i);
        }
        forEachBitIndex_i(opp[queens]) {
            dangerZone |= getLegalBishopAttacks(removedKingOcc, i);
            dangerZone |= getLegalRookAttacks(removedKingOcc, i);
        }
        forEachBitIndex_i(opp[knights]) {
            dangerZone |= knightAttacks[toTableIndex(i)];
        }
        if (side) {
            dangerZone |= ((opp[pawns] & ~files[7]) >> 7) | ((opp[pawns] & ~files[0]) >> 9);
        } else {
            dangerZone |= ((opp[pawns] & ~files[7]) << 9) | ((opp[pawns] & ~files[0]) << 7);
        }
        dangerZone |= kingAttacks[toTableIndex(opp[king])];

        if (dangerZone & me[king]) {//quick look to see if king is in check
            //dangerZone overlapping onto king, therefore I am in check

            //find pieces that are putting me in check
            u64 checkingRQ = firstPassR & (opp[rooks] | opp[queens]);
            u64 checkingBQ = firstPassB & (opp[bishops] | opp[queens]);

            u64 checkingPawn;
            if (side) {
                checkingPawn = (((me[king] << 7) & (~files[7])) |
                                ((me[king] << 9) & (~files[0]))) & opp[pawns];
            } else {
                checkingPawn = (((me[king] >> 7) & (~files[0])) |
                                ((me[king] >> 9) & (~files[7]))) & opp[pawns];
            }
            u64 checkingKnight = knightAttacks[toTableIndex(me[king])] & opp[knights];

            u64 checkAll = checkingRQ | checkingBQ | checkingPawn | checkingKnight;

            //I'm in check
            if (__builtin_popcountll(checkAll) > 1) { // more than 1 piece putting me in check, must move king rather than block/take
                legalMovesToGetOutOfCheck = 0ull;
            } else {
                legalMovesToGetOutOfCheck = checkAll;
                //only 1 piece putting in check, can either take piece or block
                forEachBitIndex_i(checkingRQ) {
                    legalMovesToGetOutOfCheck |= (getLegalRookAttacks(totalOcc, i) & firstPassR) | i;
                }
                forEachBitIndex_i(checkingBQ) {
                    legalMovesToGetOutOfCheck |= (getLegalBishopAttacks(totalOcc, i) & firstPassB) | i;
                }
            }
        } else {
            legalMovesToGetOutOfCheck = ~0ull;
        }

        firstPassR = getLegalRookAttacks(totalOcc, opp[king]);
        firstPassB = getLegalBishopAttacks(totalOcc, opp[king]);

        //remove my pieces from firstPasses, as they block queen moves out from opponent king (also opp pawns, for enpassant look)
        occMinusOwnPiecesBlockingKingR = ((me[occ]) & firstPassR) ^ totalOcc;
        occMinusOwnPiecesBlockingKingB = ((me[occ] | opp[pawns]) & firstPassB) ^ totalOcc;

        willUnblockB = 0;//reset required blocking
        willUnblockR = 0;//reset required blocking
        u64 secondPassR = 0;
        u64 secondPassB = 0;
        if (occMinusOwnPiecesBlockingKingR != firstPassR) {
            // get legal rook moves going out from king, with pieces that were blocking
            // as discovered from firstPass removed, to allow finding pieces behind them
            secondPassR = getLegalRookAttacks(occMinusOwnPiecesBlockingKingR, opp[king]);

            u64 myRookSlidersThatAreBlocked = ((me[rooks] | me[queens]) & secondPassR);


            forEachBitIndex_i(myRookSlidersThatAreBlocked) {
                willUnblockR |= (getLegalRookAttacks(occMinusOwnPiecesBlockingKingR, i) & secondPassR);
            }
        }
        if (occMinusOwnPiecesBlockingKingB != firstPassB) {
            // get legal bishop moves going out from king, with pieces that were blocking
            // as discovered from firstPass removed, to allow finding pieces behind them
            secondPassB = getLegalBishopAttacks(occMinusOwnPiecesBlockingKingB, opp[king]);

            u64 myBishopSlidersThatAreBlocked = ((me[bishops] | me[queens]) & secondPassB);

            forEachBitIndex_i(myBishopSlidersThatAreBlocked) {
                willUnblockB |= (getLegalBishopAttacks(occMinusOwnPiecesBlockingKingB, i) & secondPassB);
            }
        }
        willUnblock = willUnblockR | willUnblockB;

        if (!side) {
            willPutInCheck[pawns / 2 - 2] = (((opp[king] << 7) & (~files[7])) |
                                             ((opp[king] << 9) & (~files[0])));
        } else {
            willPutInCheck[pawns / 2 - 2] = (((opp[king] >> 7) & (~files[0])) |
                                             ((opp[king] >> 9) & (~files[7])));
        }
        willPutInCheck[bishops / 2 - 2] = firstPassB;
        willPutInCheck[knights / 2 - 2] = knightAttacks[toTableIndex(opp[king])];
        willPutInCheck[rooks / 2 - 2] = firstPassR;
        willPutInCheck[queens / 2 - 2] = firstPassR | firstPassB;
    }

    PackedMove::packedMoveType BitBoard::willEnpassCauseCheck(u64 to, u64 from) {
        u64 stolenPawn = (side) ? (to >> 8) : (to << 8);
        if (((from & willUnblock) && (to & ~willUnblock)) || to & willPutInCheck[pawns / 2 - 2])
            return PackedMove::will_check_flag;
        if (((stolenPawn & willUnblockR) && (to & ~willUnblockR)) || (stolenPawn & willUnblockB))
            return PackedMove::will_check_flag;
        else if ((opp[king] & ((side) ? (ranks[4]) : (ranks[3]))) &&
                 //king is on same rank as pawn doing enpassant, and pawn that will be captured
                 (((side) ? (ranks[4]) : (ranks[3])) & (me[queens] | me[rooks]))) {//queen or rook on same rank too
            if (getLegalRookAttacks((me[occ] | opp[occ]) ^ (stolenPawn | from), opp[king]) & (me[queens] | me[rooks])) {
                //unique case in which removing both pawns from the rank will cause a check to occur
                return PackedMove::will_check_flag;
            }
        }
        return 0;
    }


    void BitBoard::performMove(const cpp_client::chess::Move &move) {
        PackedMove::packedMoveType m = 0;
        u64 from = toBitIndex(move->from_file, move->from_rank);
        u64 to = toBitIndex(move->to_file, move->to_rank);
        m |= toTableIndex(from);
        m |= toTableIndex(to) << 8;

        if (move->promotion != "") {
            //having a promotion guarantees this is a pawn
            if (move->promotion == "Queen") {
                m |= queens << PackedMove::promotion_shift;
            } else if (move->promotion == "Knight") {
                m |= knights << PackedMove::promotion_shift;
            } else if (move->promotion == "Rook") {
                m |= rooks << PackedMove::promotion_shift;
            } else if (move->promotion == "Bishop") {
                m |= bishops << PackedMove::promotion_shift;
            } else {
                std::cout << "unable to determine promotion type\n";
            }
        } else {
            if (opp[pawns] & from) {
                if (doLShift(to, rankDirection[side] * 16) == from) {//double forward
                    m |= PackedMove::double_pawn_forward_flag;
                } else if (doLShift(to, rankDirection[side] * 8) == from) {//single forward
                } else if (_[side | occ] & to) {//capture
                } else {//enpassant
                    m |= PackedMove::en_passant_flag;
                }
            } else if (opp[king] & from) {
                if (!(to & kingAttacks[toTableIndex(from)])) {
                    //king moved double, therefore it castled
                    if (to & whiteQueenCastleLanding) {
                        m |= PackedMove::queen_castle_mask;
                    } else if (to & whiteKingCastleLanding) {
                        m |= PackedMove::king_castle_mask;
                    } else if (to & blackQueenCastleLanding) {
                        m |= PackedMove::queen_castle_mask;
                    } else if (to & blackKingCastleLanding) {
                        m |= PackedMove::king_castle_mask;
                    } else {
                        std::cout << "did not determine type of castle\n";
                    }
                }
            }
        }
        if (me[occ] & to) {
            //move captures a piece
            unsigned capturedPiece;
            for (capturedPiece = pawns; !(to & me[capturedPiece]); capturedPiece += 2) {
                /*empty loop*/
            }
            m |= capturedPiece << PackedMove::captured_shift;
        }

        unsigned pieceMoved;
        for (pieceMoved = king; !(from & opp[pieceMoved]); pieceMoved += 2) {
            /*empty loop*/
        }
        m |= pieceMoved << PackedMove::piece_moved_shift;

        switchSides();
        PackedMove mov = {m};
        performMove(mov);
        switchSides();
    }

    void BitBoard::performMove(PackedMove &move) {
        me = &_[side];
        opp = &_[1 - side];

        u64 from = move.from();
        u64 to = move.to();

        unsigned from_TableIndex = move.fromTableIndex();
        unsigned to_TableIndex = move.toTableIndex();
        zobristHash ^= pieceZobristMasks[(side | move.pieceMoved()) - 2][from_TableIndex]; //remove moved piece's from mask
        zobristHash ^= pieceZobristMasks[(side | move.pieceMoved()) - 2][to_TableIndex]; //add to moved piece's to mask

        if (from & notYetMoved) {
            for (int bw = 0; bw < 2; bw++) {
                for (int kq = 0; kq < 2; kq++) {
                    if (from & CastleMasks[bw][kq].castle)//this move will remove possibilty for a castle
                        zobristHash ^= castlingZobristMasks[bw * 2 + kq];
                }
            }
            move.m |= PackedMove::notYetMoved_from_mask;
        }
        if (to & notYetMoved) {
            for (int bw = 0; bw < 2; bw++) {
                for (int kq = 0; kq < 2; kq++) {
                    if (to & CastleMasks[bw][kq].castle)//this move will remove possibility for a castle
                        zobristHash ^= castlingZobristMasks[bw * 2 + kq];
                }
            }
            move.m |= PackedMove::notYetMoved_to_mask;
        }

        notYetMoved &= ~from;

        me[occ] ^= from | to;//change positions in occupancy
        me[move.pieceMoved()] ^= from | to;//move piece in its own pieces mask

        if (enpassanter)//get rid of enpassant flag
            zobristHash ^= enPassantFileZobristMasks[toFileIndex(enpassanter)];
        enpassanter = 0ull;

        if (__builtin_expect(move.didCastle(), 0)) {
            //castled
            if (move.didKingSideCastle()) {
                //king side castle
                me[rooks] ^= CastleMasks[side][0].rookLanding | CastleMasks[side][0].rookStart;
                me[occ] ^= CastleMasks[side][0].rookLanding | CastleMasks[side][0].rookStart;
                zobristHash ^= pieceZobristMasks[(side | rooks) - 2][toTableIndex(CastleMasks[side][0].rookLanding)];
                zobristHash ^= pieceZobristMasks[(side | rooks) - 2][toTableIndex(CastleMasks[side][0].rookStart)];
            } else {
                //queen side castle
                me[rooks] ^= CastleMasks[side][1].rookLanding | CastleMasks[side][1].rookStart;
                me[occ] ^= CastleMasks[side][1].rookLanding | CastleMasks[side][1].rookStart;
                zobristHash ^= pieceZobristMasks[(side | rooks) - 2][toTableIndex(CastleMasks[side][1].rookLanding)];
                zobristHash ^= pieceZobristMasks[(side | rooks) - 2][toTableIndex(CastleMasks[side][1].rookStart)];
            }
            hasCastled[side] = true;
            return;//couldn't have captured anything
        } else if (__builtin_expect(move.isDoublePawnForward(), 0)) {
            //pawn moved forward two ranks
            u64 possibleEnPass = (to > from) ? (to >> 8) : (to << 8);
            if (side) {
                if (((((~files[0]) & possibleEnPass) << 7) & _[black | pawns]) ||
                    ((((~files[7]) & possibleEnPass) << 9) & _[black | pawns])) {
                    enpassanter = possibleEnPass;
                    zobristHash ^= enPassantFileZobristMasks[toFileIndex(possibleEnPass)];
                }
            } else {
                if (((((~files[0]) & possibleEnPass) >> 9) & _[white | pawns]) ||
                    ((((~files[7]) & possibleEnPass) >> 7) & _[white | pawns])) {
                    enpassanter = possibleEnPass;
                    zobristHash ^= enPassantFileZobristMasks[toFileIndex(possibleEnPass)];

                }
            }
            return;//couldn't have captured anything
        } else if (__builtin_expect(move.didEnPassant(), 0)) {
            u64 stolenPawn = (to > from) ? (to >> 8) : (to << 8);
            opp[occ] ^= stolenPawn;//remove stolen pawn from opponents occupancy
            opp[pawns] ^= stolenPawn;//remove stolen pawn from opponents pawns mask
            materialBalance -= materialBalanceTable[(1 - side) | pawns];
            zobristHash ^= pieceZobristMasks[((1 - side) | pawns) - 2][toTableIndex(stolenPawn)];
            return;
        } else if (__builtin_expect(move.didPromote(), 0)) {
            //move is a promotion
            me[pawns] ^= to;//remove the pawn that we just moved from pawns mask
            me[move.promotionType()] ^= to;//add new piece to its mask
            materialBalance -= materialBalanceTable[(side) | pawns];
            materialBalance += materialBalanceTable[(side) | move.promotionType()];
            zobristHash ^= pieceZobristMasks[(side | move.pieceMoved()) - 2][to_TableIndex];//get rid of pawns mask
            zobristHash ^= pieceZobristMasks[(side | move.promotionType()) - 2][to_TableIndex];//add promotions mask
        }

        if (move.didCapture()) {
            opp[occ] ^= to;//removed captured piece opponent occupancy
            opp[move.pieceCaptured()] ^= to;//remove captured piece opponents pieces mask
            notYetMoved &= ~to;//remove the captured piece from not yet moved
            materialBalance -= materialBalanceTable[(1 - side) | move.pieceCaptured()];
            zobristHash ^= pieceZobristMasks[((1 - side) | move.pieceCaptured()) - 2][to_TableIndex];
        }
    }

    void BitBoard::undoMove(const PackedMove &move) {
        me = &_[side];
        opp = &_[1 - side];

        u64 from = move.from();
        u64 to = move.to();

        unsigned from_TableIndex = move.fromTableIndex();
        unsigned to_TableIndex = move.toTableIndex();
        zobristHash ^= pieceZobristMasks[(side | move.pieceMoved()) - 2][from_TableIndex]; //add back in from mask
        zobristHash ^= pieceZobristMasks[(side | move.pieceMoved()) - 2][to_TableIndex]; //remove to mask

        if (move.didToMove()) {
            notYetMoved |= to;
            for (int bw = 0; bw < 2; bw++) {
                for (int kq = 0; kq < 2; kq++) {
                    if ((notYetMoved & CastleMasks[bw][kq].castle) && (to & CastleMasks[bw][kq].castle))
                        zobristHash ^= castlingZobristMasks[bw * 2 + kq]; //added ability for a castle move back
                }
            }
        }

        if (move.didFromMove()) {
            notYetMoved |= from;
            for (int bw = 0; bw < 2; bw++) {
                for (int kq = 0; kq < 2; kq++) {
                    if ((notYetMoved & CastleMasks[bw][kq].castle) && (from & CastleMasks[bw][kq].castle))
                        zobristHash ^= castlingZobristMasks[bw * 2 + kq]; //added ability for a castle move back
                }
            }
        }

        me[occ] ^= from | to;//change positions in occupancy
        me[move.pieceMoved()] ^= from | to;//move piece in its own pieces mask

        if (enpassanter)//get rid of enpassant flag
            zobristHash ^= enPassantFileZobristMasks[toFileIndex(enpassanter)];
        enpassanter = 0ull;

        if (__builtin_expect(move.didCastle(), 0)) {
            //castled
            if (move.didKingSideCastle()) {
                //king side castle
                me[rooks] ^= CastleMasks[side][0].rookLanding | CastleMasks[side][0].rookStart;
                me[occ] ^= CastleMasks[side][0].rookLanding | CastleMasks[side][0].rookStart;
                zobristHash ^= pieceZobristMasks[(side | rooks) - 2][toTableIndex(CastleMasks[side][0].rookLanding)];
                zobristHash ^= pieceZobristMasks[(side | rooks) - 2][toTableIndex(CastleMasks[side][0].rookStart)];
            } else {
                //queen side castle
                me[rooks] ^= CastleMasks[side][1].rookLanding | CastleMasks[side][1].rookStart;
                me[occ] ^= CastleMasks[side][1].rookLanding | CastleMasks[side][1].rookStart;
                zobristHash ^= pieceZobristMasks[(side | rooks) - 2][toTableIndex(CastleMasks[side][1].rookLanding)];
                zobristHash ^= pieceZobristMasks[(side | rooks) - 2][toTableIndex(CastleMasks[side][1].rookStart)];
            }
            hasCastled[side] = false;
            return;//couldn't have captured anything
        } else if (__builtin_expect(move.isDoublePawnForward(), 0)) {
            //pawn moved forward two ranks
            //don't care about enpassanter in undoing move
            return;//couldn't have captured anything
        } else if (__builtin_expect(move.didEnPassant(), 0)) {
            u64 stolenPawn = (to > from) ? (to >> 8) : (to << 8);
            opp[occ] ^= stolenPawn;//return stolen pawn from opponents occupancy
            opp[pawns] ^= stolenPawn;//return stolen pawn from opponents pawns mask
            materialBalance += materialBalanceTable[(1 - side) | pawns];
            zobristHash ^= pieceZobristMasks[((1 - side) | pawns) - 2][toTableIndex(stolenPawn)];
            return;
        } else if (__builtin_expect(move.didPromote(), 0)) {
            //move is a promotion
            me[pawns] ^= to;//return the pawn that we just moved from pawns mask
            me[move.promotionType()] ^= to;//remove the promoted piece
            materialBalance += materialBalanceTable[(side) | pawns];
            materialBalance -= materialBalanceTable[(side) | move.promotionType()];
            zobristHash ^= pieceZobristMasks[(side | move.pieceMoved()) - 2][to_TableIndex];//add pawns mask
            zobristHash ^= pieceZobristMasks[(side | move.promotionType()) - 2][to_TableIndex];//remove promotions mask
        }

        if (move.didCapture()) {
            opp[occ] ^= to;//return captured piece opponent occupancy
            opp[move.pieceCaptured()] ^= to;//return captured piece opponents pieces mask
            materialBalance += materialBalanceTable[(1 - side) | move.pieceCaptured()];
            zobristHash ^= pieceZobristMasks[((1 - side) | move.pieceCaptured()) - 2][to_TableIndex];
        }
    }

    void BitBoard::switchSides() {
        side = (color) (1 - side);
        me = &_[side];
        opp = &_[1 - side];
        zobristHash ^= whitesTurnZobristMask;
    }


}