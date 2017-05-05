//
// Created by Shawn Roach on 4/12/2017.
//

#include "Evaluation.h"
#include "BitBoard.h"
#include "BitMath.h"

extern BitBoard::BitBoard bitboard;
namespace BitBoard {
    extern color startSide;

#define popCount(x) __builtin_popcountll(x)

    constexpr u64 whitePawnKQspots = (1ull << 11 | 1ull << 12);
    constexpr u64 blackPawnKQspots = (1ull << 51 | 1ull << 52);
    constexpr u64 Qspots[2] = {1ull << 59 /*black queen*/, 1ull << 3 /*white queen*/};
    constexpr u64 Bspots[2] = {1ull << 58 | 1ull << 61, // black bishop spots
                               1ull << 2 | 1ull << 5}; // white bishop spots
    constexpr u64 Nspots[2] = {1ull << 57 | 1ull << 62, // black knight spots
                               1ull << 1 | 1ull << 6}; // white knight spots
    constexpr u64 Kspots[2] = {1ull << 60, //black king spot
                               1ull << 4};//white king spot
    constexpr u64 Rspots[2] = {1ull << 56 | 1ull << 63, // black rooks spots
                               1ull << 0 | 1ull << 7}; // white rooks spots



    constexpr u64 whiteTiles = 0b1010101010101010101010101010101010101010101010101010101010101010ull;
    constexpr u64 blackTiles = 0b0101010101010101010101010101010101010101010101010101010101010101ull;

    constexpr u64 filesWithNeighborFiles[8] = {
            files[0] | files[1],
            files[0] | files[1] | files[2],
            files[1] | files[2] | files[3],
            files[2] | files[3] | files[4],
            files[3] | files[4] | files[5],
            files[4] | files[5] | files[6],
            files[5] | files[6] | files[7],
            files[6] | files[7],
    };

    constexpr int coarseGrain = 3;

    score_t evaluate() {
        //https://books.google.com/books?id=9-3pBwAAQBAJ&pg=PA97&lpg=PA97&dq=chess+%22king+tropism%22&source=bl&ots=5iDJCFlOOd&sig=or7k6qk6h-YYrdBy4WFfWVPy70Y&hl=en&sa=X&ved=0ahUKEwir34iyq6DTAhWF64MKHel4AiAQ6AEIPzAD#v=onepage&q=chess%20%22king%20tropism%22&f=false

        //todo return stalemate on insufficient material

        bool needflip = (startSide != bitboard.side);
        if (needflip)
            bitboard.switchSides();

        const auto *const opp = bitboard.opp;
        const auto *const me = bitboard.me;

        score_t pawnstrucure = 0;

        //encourage moving pawns in front of king and queen
        if (bitboard.side) {
            pawnstrucure -= 8 * popCount(me[pawns] << 8 & opp[pawns]);//penalize pawn rams for restricting movement
            pawnstrucure -= 15 * popCount(me[pawns] & ~(me[pawns] >> 7 | me[pawns] >> 9));//pawns not protected by other pawns
            pawnstrucure -= 15 * popCount(me[pawns] & whitePawnKQspots);//encourage moving king & queen pawns
            //pawnstrucure += 13*popCount(me[pawns] & (me[pawns] << 7 | me[pawns] << 9));//num pawns protected by other pawns
        } else {
            pawnstrucure -= 8 * popCount(me[pawns] >> 8 & opp[pawns]);//penalize pawn rams for restricting movement
            pawnstrucure -= 15 * popCount(me[pawns] & ~(me[pawns] << 7 | me[pawns] << 9));//pawns not protected by other pawns
            pawnstrucure -= 15 * popCount(me[pawns] & blackPawnKQspots);//encourage moving king & queen pawns
            //pawnstrucure += 13*popCount(me[pawns] & (me[pawns] >> 7 | me[pawns] >> 9)); //num pawns protected by other pawns
        }
        if (popCount(me[pawns] == 8))//to many pawns, add small punishment to encourage opening up a file
            pawnstrucure -= 10;

        u64 passedPawns = 0;
        for (int file = 0; file < 7; file++) {
            u64 pawnsOnFile = me[pawns] & files[file];
            if (pawnsOnFile) { //pawn found in file
                u64 possibleBlockingPawns = opp[pawns] & filesWithNeighborFiles[file];

                if (bitboard.side) {
                    if (pawnsOnFile << 2 > possibleBlockingPawns) {
                        //there is a passed pawn on this file
                        u64 farthestPawn = 1ull << (63 - __builtin_clzll(pawnsOnFile));
                        auto farthestPawnRank = toRankIndex(farthestPawn);
                        //quadratic addition instad of linear to encourage advancing a single passed pawn instead of all of them
                        pawnstrucure += farthestPawnRank * farthestPawnRank;
                        passedPawns |= farthestPawn;
                    }
                } else {
                    //need to fetch pawn with lowest set bit in pawns on file
                    u64 farthestPawn = pawnsOnFile & -pawnsOnFile;//1ull << toTableIndex(pawnsOnFile);
                    if (possibleBlockingPawns == 0 || farthestPawn >> 2 < (possibleBlockingPawns & -possibleBlockingPawns)) {
                        //there is a passed pawn on this file
                        auto farthestPawnRank = 7 - toRankIndex(farthestPawn);
                        //quadratic addition instead of linear to encourage advancing a single passed pawn instead of all of them
                        pawnstrucure += farthestPawnRank * farthestPawnRank;
                        passedPawns |= farthestPawn;
                    }
                }
            }
        }

        score_t developmentStructure = 0;

        //penalize for still having bishops/knights in the back row
        developmentStructure -= 10 * popCount((me[bishops] | me[knights]) & ranks[bitboard.side ? 0 : 7]);

        //incite castling if queen on board, thus large bonus to either capture enemy queen, or castle for protection
        if (opp[queens]) {
            if (bitboard.hasCastled[bitboard.side])
                developmentStructure += 10; // big bonus for having castled
            else {
                bool canKingsideCastle = ((bitboard.notYetMoved & CastleMasks[bitboard.side][0].castle) ==
                                          CastleMasks[bitboard.side][0].castle);
                bool canQueenSideCastle = ((bitboard.notYetMoved & CastleMasks[bitboard.side][1].castle) ==
                                           CastleMasks[bitboard.side][1].castle);
                if (canKingsideCastle && canQueenSideCastle)
                    developmentStructure -= 24;
                else if (canKingsideCastle)
                    developmentStructure -= 40;
                else if (canQueenSideCastle)
                    developmentStructure -= 80;
                else
                    developmentStructure -= 120;
            }
        }

        //discourage queen from moving until other pieces have moved, as queen is best saved for mid/late game
        if (!(me[queens] & Qspots[bitboard.side])) {
            developmentStructure -= 8 * (popCount((me[bishops] & Bspots[bitboard.side]) |
                                                  (me[knights] & Nspots[bitboard.side]) |
                                                  (me[rooks] & Rspots[bitboard.side]) |
                                                  (me[king] & Kspots[bitboard.side])));
        }


        //penalty for surviving bishops that are heavily hindered by pawns on same color tile
        forEachBitIndex_i(me[bishops]) {
            if (i & whiteTiles) // bishop on white tile
                developmentStructure -= 8 * popCount(me[pawns] & whiteTiles);
            else //bishop on black tile
                developmentStructure -= 8 * popCount(me[pawns] & blackTiles);
        }


        score_t kingTropism = 0;
        int oppKingRank = toRankIndex(opp[king]);
        int oppKingFile = toFileIndex(opp[king]);
        forEachBitIndex_i(me[rooks]) {
            if (i & ranks[bitboard.side ? 7 : 0]) //encourage having rook in opponents back row
                developmentStructure += 22;

            int fileIndex = toFileIndex(i);

            if (!(me[pawns] & files[fileIndex])) {//none of my pawns on same file as rook
                if (opp[pawns] & files[fileIndex])//opponent pawn on same file as rook
                    developmentStructure += 4;
                else
                    developmentStructure += 10;
            } else if (passedPawns & files[fileIndex]) {
                if (bitboard.side) {
                    if (i < (passedPawns & files[fileIndex]))
                        pawnstrucure += 25;//rook is behind passed pawn
                } else {
                    if (i > (passedPawns & files[fileIndex]))
                        pawnstrucure += 25;//rook is behind passed pawn
                }
            }

            kingTropism -= 2 * std::min(std::abs(oppKingRank - (int) toRankIndex(i)),
                                        std::abs(oppKingFile - fileIndex));
        }
        forEachBitIndex_i(me[knights]) {
            kingTropism += 5 - std::abs(oppKingRank - (int) toRankIndex(i)) -
                           std::abs(oppKingFile - (int) toFileIndex(i));
        }
        forEachBitIndex_i(me[queens]) {
            kingTropism -= std::min(std::abs(oppKingRank - (int) toRankIndex(i)),
                                    std::abs(oppKingFile - (int) toFileIndex(i)));
        }

        //todo add protected pieces bonus.

        if (needflip)
            bitboard.switchSides();

        score_t total = (score_t) (bitboard.materialBalance + pawnstrucure + developmentStructure);
        total = (total >> coarseGrain) << coarseGrain;
        return total;
        //return (score_t) (100 * bitboard.materialBalance + pawnstrucure + developmentStructure);

    }

    score_t fast_evaluate() {
        return (score_t) (bitboard.materialBalance);
    }

#ifdef oldEvaluater
    score_t OldEvaluate() {
        bool needflip = (startSide != bitboard.side);
        if (needflip)
            bitboard.switchSides();

        const auto *const opp = bitboard.opp;
        const auto *const me = bitboard.me;

        score_t mepiecevalues = 0;
        score_t opppiecevalues = -bitboard.materialBalance;

        score_t pawnstrucure = 0;
        if (bitboard.side) {
            pawnstrucure = __builtin_popcountll(me[pawns] & (me[pawns] << 7 | me[pawns] << 9));

        } else {
            pawnstrucure = __builtin_popcountll(me[pawns] & (me[pawns] >> 7 | me[pawns] >> 9));
        }

        score_t defensiveCastleStructure = 0;
        int8_t canCastle = 0;
        for (int x = 0; x < 2; x++) {
            if ((bitboard.notYetMoved & CastleMasks[bitboard.side][x].castle) == CastleMasks[bitboard.side][x].castle) {
                canCastle++;
            }
            defensiveCastleStructure -= __builtin_popcountll(me[occ] & CastleMasks[bitboard.side][x].empties);
        }
        if (canCastle == 0) {
            defensiveCastleStructure -= 3;
        } else if (canCastle == 2) {
            defensiveCastleStructure -= 1; //get atleast one of the rooks moving out
        }

        bitboard.switchSides();
        bitboard.lookForCheckInducingMovements();
        if (~bitboard.legalMovesToGetOutOfCheck) {
            //opponent in check
            //mepiecevalues += 6;
        }

        score_t oppDanga = __builtin_popcountll(bitboard.dangerZone & opp[occ]);// num opp pieces attackable by me
        score_t myProtected = __builtin_popcountll(bitboard.dangerZone & me[occ]);//num my pieces protected by others of my pieces
        bitboard.switchSides();

        score_t pawnDists = 0;
        forEachBitIndex_i(me[pawns]) {
            if (bitboard.side) {
                pawnDists += toRankIndex(i) - 1;
            } else {
                pawnDists += 6 - toRankIndex(i);
            }
        }
        pawnDists /= 6;

        if (needflip)
            bitboard.switchSides();

        return (score_t) (3 * mepiecevalues - 3 * opppiecevalues + 1 * pawnstrucure /*- myDanga*/ + oppDanga + pawnDists + myProtected +
                          defensiveCastleStructure);
    }

#endif
}