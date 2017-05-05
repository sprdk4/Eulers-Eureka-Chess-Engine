//
// Created by Shawn Roach on 3/14/2017.
//

#include <algorithm>
#include <iostream>
#include <sstream>
#include <unordered_set>
#include <cmath>
#include "MoveSelection.h"
#include "Move.h"
#include "BitBoard.h"
#include "BitMath.h"
#include "ScopedBenchmark.h"
#include "TranspositionTable.h"
#include "HistoryTable.h"
#include "Evaluation.h"
#include "OpeningTable.h"

extern BitBoard::BitBoard bitboard;
extern std::stringstream toAddToBook;
extern BitBoard::OpeningTable openingTable;
int64_t grandNodesExplored = 0;
int64_t grandMovesGenerated = 0;
namespace BitBoard {
    std::ostream &operator<<(std::ostream &out, const PackedMove &e) {
        std::string fromFile = toFile(e.from());
        std::string toFile_ = toFile(e.to());
        int fromRank = toRank(e.from());
        int toRank_ = toRank(e.to());
        out << fromFile << fromRank << " to " << toFile_ << toRank_ << '\n';
        return out;
    }

    TranspositionTable trans;
    PackedMove bestestMove;
    std::unordered_set<u64> prevStates;
    int64_t nodesExplored = 0;
    int64_t movesGenerated = 0;
    depth_t startDepth;
    uint8_t turnNo = 0;
    color startSide;

    void init() {
        for (u64 x = 0ull; x < trans.tableLength; x++) {
            // prevent crash where a tranposition entry has never been set but it still believes it found a proper fit, and attempts to
            // perform the non existent move, which is set to 0 causing a crash.
            // Extremely rare, so the cost of deeming such moves as checkmates is worth the error handling
            auto& entry=trans.getTransposition(x);
            entry.hash=0;
            entry.depthLeft=100;
            entry.score=-checkmate;
            entry.turnUsed=0;
        }
    }

    score_t mtdf(score_t g, depth_t depthLeft) {
        //todo implement best node search, http://www.bjmc.lu.lv/fileadmin/user_upload/lu_portal/projekti/bjmc/Contents/770_7.pdf
        //https://people.csail.mit.edu/plaat/mtdf.html
        score_t upper = std::numeric_limits<score_t>::max();
        score_t lower = std::numeric_limits<score_t>::min();
        score_t beta;
        while (lower < upper) {
            if (g == lower)
                beta = g + (score_t) 1;
            else
                beta = g;
            g = alphaBetaMaxTran(beta - (score_t) 1, beta, depthLeft, {0});
            if (g < beta)
                upper = g;
            else
                lower = g;
        }
        return g;
    }

    PackedMove getBestMove(const double &time_remaining) {
        //http://adamsccpages.blogspot.com/p/chess-programming-resources.html
        turnNo++;
        startSide = bitboard.side;
        //setting a required min depth helps achiece a more bell curve in time usage, as recommended here:
        // http://chessprogramming.wikispaces.com/Time+Management
        depth_t minDepth;
        double cutOffTime;
        double expectedBranchingFactor = bitboard.getLegalMoves().length * 0.5;
        if (time_remaining > 60e10) {//more than 10 minutes (600 seconds)
            minDepth = 9;
            cutOffTime = time_remaining / 25;
        } else if (time_remaining > 42e10) {//more than 7 minutes (420 seconds)
            minDepth = 8;
            cutOffTime = time_remaining / 25;
        } else if (time_remaining > 1e11) {//more than 100 secs left
            minDepth = 8;
            cutOffTime = time_remaining / 50;
        } else if ((time_remaining > 4e10)) {// between 100 and 40 secs left
            minDepth = 7;
            cutOffTime = time_remaining / 120;
        } else { // under 40 secs left
            //at this point, I consider making a somewhat dumb move better, than running out of time
            minDepth = 4;
            cutOffTime = time_remaining / 300;
        }
        auto start = std::chrono::high_resolution_clock::now();
        auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(start - start);
        auto difBetween = now;


        auto zobristHash = bitboard.zobristHash;

        auto entry = openingTable[zobristHash];
        if (entry.zobristHash == zobristHash) {
            bestestMove.m = entry.bestMoveFound;
            std::cout << "entry found in opening table\n";
            bitboard.performMove(bestestMove);
            if (prevStates.find(bitboard.zobristHash) != prevStates.end() || entry.scoreComputed == stalemate) {
                std::cout << "not using opening table entry as it would cause repetition\n";
                entry.timePredictedToSearchNextDepth = 0;
                entry.depthLookedTo = 0;
            }
            bitboard.undoMove(bestestMove);
        }
        //minDepth = 1; //strips ability to start at an initial depth above 1, to correspond with iterative deepening requirement
        depth_t onDepth = 1;
        int64_t totalNodesExplored = 0;
        int64_t totalMovesGenerated = 0;
        score_t max;
        score_t nextGuess = 0;
        //todo remove the *.9 when submitting for competivity, thats there for creating opening tables right now
        if (entry.timePredictedToSearchNextDepth * .9 < cutOffTime || entry.depthLookedTo < minDepth) {
            SCOPED_BENCHMARK("alpha beta");
            do {
                nodesExplored = 0;
                movesGenerated = 0;

                //calling alpaBetaMax sets global bestestMove
                max = mtdf(nextGuess, startDepth = onDepth);
                nextGuess = max;
                //max = alphaBetaMaxTran(std::numeric_limits<score_t>::min(), std::numeric_limits<score_t>::max(), startDepth = onDepth,{0});
                //max = alphaBetaMax(std::numeric_limits<score_t>::min(), std::numeric_limits<score_t>::max(), startDepth = onDepth);

                totalNodesExplored += nodesExplored;
                totalMovesGenerated += movesGenerated;
                onDepth++;

                // compute current time since start
                auto newnow = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start);
                // compute time that the current iteration took
                auto newDifBetween = newnow - now;
                if (newDifBetween.count() == 0)
                    newDifBetween = newnow;

                std::cout << "ondepth: " << (int32_t) (onDepth - 1) << " difBetween: " << newDifBetween.count()
                          << "\tminimax result: " << max
                          << "\tnodesExplored: " << nodesExplored << '\n';
                if (onDepth > 1 && difBetween.count() > 0 && newDifBetween.count() > 0 && newDifBetween >= difBetween) {
                    //try to predict the effective branching factor that will occur in the next iteration
                    expectedBranchingFactor = ((double) (newDifBetween.count())) / difBetween.count();
                    expectedBranchingFactor = std::max(expectedBranchingFactor + 1, 3.0);//finagling on expected branching factor
                    //std::cout << "\t   predicting: " << (int64_t) (newDifBetween * expectedBranchingFactor).count()
                    //          << "\t next expectedBranch: "
                    //          << expectedBranchingFactor << '\n';

                    // also a legitimate algorithm for predicting next iterations factor, however I believe the current version is more accurate
                    // I could also decide to average these two algorithms together too, and see what that gets me
                    // side note, this method tends to be larger predicitions than the current method, which may be better if allotted time
                    //   per turn is increased significantly
                    double otherBranching = std::log(nodesExplored) / std::log(onDepth - 1);
                    otherBranching = std::max(otherBranching, 4.5);
                    //std::cout << "\t   predicting: " << (int64_t) (newDifBetween * otherBranching).count() << "\tother expectedBranch: "
                    //          << otherBranching << '\n';

                    if (expectedBranchingFactor > 2 * otherBranching)
                        expectedBranchingFactor = otherBranching;
                    expectedBranchingFactor = (expectedBranchingFactor * 0.5 + 0.5 * otherBranching);

                    //std::cout << "\t   predicting: " << (int64_t) (newDifBetween * expectedBranchingFactor).count()
                    //          << "\tfinal expectedBranch: "
                    //          << expectedBranchingFactor << '\n';

                }
                difBetween = newDifBetween;
                now = newnow;
                std::cout << std::flush;
            } while (
                    ((now + difBetween * expectedBranchingFactor).count() < cutOffTime // current time plus predicted time of next iteration
                     || onDepth <= minDepth) && /*onDepth < 9 &&*/ max < checkmate - onDepth && max > -checkmate && onDepth < 30);

            if (onDepth - 1 <= entry.depthLookedTo) {
                //the entry in the opening table was better than the move I created
                std::cout << "tried to find something better than in opening table but failed\n";
                bestestMove.m = entry.bestMoveFound;
            }

            toAddToBook << zobristHash << ' ' << (uint32_t) bestestMove.m << ' ' << onDepth - 1 << ' ' << max << ' '
                        << (uint64_t) (now + difBetween * expectedBranchingFactor).count() << '\n';
        } else {
            std::cout << "used opening table entry\n";
        }

        std::cout << "took:   "
                  << std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count()
                  << " ns\n";
        std::cout << "out of: " << (u64) cutOffTime << " ns\n";
        std::cout << "nodes explored:  " << totalNodesExplored << '\n';
        std::cout << "moves generated: " << totalMovesGenerated << '\n';
        grandMovesGenerated += totalMovesGenerated;
        grandNodesExplored += totalNodesExplored;


        if (bestestMove.didCapture() || bestestMove.pieceMoved() == pawns)
            prevStates.clear(); //impossible for current past states to be reachable again, after a capture or pawn movement

        bitboard.performMove(bestestMove);
        prevStates.insert(bitboard.zobristHash);//todo get rid of prevstates, storing stalemates in transpositiont table now
        bitboard.switchSides();
        auto &transPos = trans.getTransposition(bitboard.zobristHash);
        transPos.score = stalemate;
        transPos.flag = EXACT;
        transPos.depthLeft = 100;
        transPos.hash = (uint32_t) bitboard.zobristHash;
        bitboard.switchSides();
        bitboard.undoMove(bestestMove);

        //history.zeroTable();
        return bestestMove;
    }

    score_t alphaBetaMaxTran(score_t alpha, score_t beta, depth_t depthleft, PackedMove lastMove) {

        if (depthleft == 0) {
            return alphaBetaMaxQuiescence(alpha, beta, depthleft, lastMove);
            //return evaluate();
        }
        nodesExplored++;

        score_t bestScore = -checkmate - 1;
        PackedMove bestMove = {0};
        PackedMove transMove = {0};

        uint32_t zobristhash = (uint32_t) bitboard.zobristHash;
        auto &tranPos = trans.getTransposition(bitboard.zobristHash);

        if (tranPos.hash == zobristhash) {
            tranPos.turnUsed = turnNo;
            transMove.m = tranPos.bestMove & PackedMove::remove_notYetMoved_mask;
            if (tranPos.depthLeft >= depthleft) {
                //the score in transposition table is stronger than if I let this minimax continue normally
                if (tranPos.flag == EXACT) {
                    bestestMove = transMove;
                    return tranPos.score;
                } else if (tranPos.flag == ABOVE_BETA) {
                    if (tranPos.score >= beta) {
                        bestestMove = transMove;
                        return tranPos.score;
                    }
                } else if (tranPos.flag == BELOW_ALPHA) {
                    if (tranPos.score <= alpha) {
                        bestestMove = transMove;
                        return tranPos.score;
                    }
                }
            }

            //unable to return after looking at transposition table
            //check out the move stored in the transposition table
            bitboard.performMove(transMove);
            score_t score;
            //if (depthleft >= startDepth - 2 && transMove < PackedMove::double_pawn_forward_flag &&
            //    prevStates.find(bitboard.zobristHash) != prevStates.end()) {
            //    nodesExplored++;
            //    score = stalemate;
            //} else {
            bitboard.switchSides();
            score = alphaBetaMinTran(alpha, beta, depthleft - (depth_t) 1, transMove);
            bitboard.switchSides();
            //}
            bitboard.undoMove(transMove);
            if (score > bestScore) {
                bestMove = transMove;
                bestScore = score;
            }
            if (score >= beta) {
                transposition newTrans = {zobristhash, (uint32_t) bestMove, bestScore, ABOVE_BETA, depthleft, turnNo};
                if (tranPos.shouldOverwrite(newTrans))
                    tranPos = newTrans;
                history.updateHistory(bestMove, depthleft);
                bestestMove = bestMove;
                return bestScore;   // fail hard beta-cutoff
            }
            if (score > alpha) {
                alpha = score;
            }
            history.addHistoryToMove(transMove);
        }

        auto moves = bitboard.getLegalMoves();
        movesGenerated += moves.length;
        if (moves.empty()) {
            //no moves were created
            if (!(~bitboard.legalMovesToGetOutOfCheck)) {
                //stalemate
                return stalemate;
            } else {
                //checkmate
                return -checkmate;
            }
        }

        std::make_heap(moves.begin(), moves.end());
        while (moves.length) {
            auto &e = moves[0];
            //std::pop_heap(moves.begin(), moves.end());
            //auto &e = moves[--moves.length];
            if (e == transMove) {
                std::pop_heap(moves.begin(), moves.end());
                --moves.length;
                continue;//already looked at this move
            }
            bitboard.performMove(e);
            score_t score;
            if (depthleft >= startDepth - 2 && e < PackedMove::double_pawn_forward_flag && prevStates.size() > 0 &&
                prevStates.find(bitboard.zobristHash) != prevStates.end()) {
                // this is the root of the tree, and I am about to explore a move that has already been generated previously in the game
                // distinctive emphasis: previously in the game, not previously in exploration
                // this prevents three-fold repetition.
                nodesExplored++;
                score = stalemate;
            } else {
                bitboard.switchSides();
                score = alphaBetaMinTran(alpha, beta, depthleft - (depth_t) 1, e);
                bitboard.switchSides();
            }
            bitboard.undoMove(e);

            if (score > bestScore) {
                bestMove = e;
                bestScore = score;
            }
            if (score >= beta) {
                break;
            }
            if (score > alpha) {
                alpha = score; // alpha acts like max in MiniMax
            }
            std::pop_heap(moves.begin(), moves.end());
            --moves.length;
        }

        transposition newTrans = {zobristhash, (uint32_t) bestMove, bestScore, EXACT, depthleft, turnNo};
        if (bestScore >= beta) {
            newTrans.flag = ABOVE_BETA;
        } else if (bestScore <= alpha) {
            newTrans.flag = BELOW_ALPHA;
        }
        if (tranPos.shouldOverwrite(newTrans))
            tranPos = newTrans;

        bestestMove = bestMove;
        history.updateHistory(bestMove, depthleft);
        return bestScore;
    }

    score_t alphaBetaMinTran(score_t alpha, score_t beta, depth_t depthleft, PackedMove lastMove) {
        if (depthleft == 0) {
            return alphaBetaMinQuiescence(alpha, beta, depthleft, lastMove);
            //return evaluate();
        }
        nodesExplored++;

        score_t bestScore = checkmate + 1;
        PackedMove bestMove = {0};
        PackedMove transMove = {0};

        uint32_t zobristhash = (uint32_t) bitboard.zobristHash;
        auto &tranPos = trans.getTransposition(bitboard.zobristHash);

        if (tranPos.hash == zobristhash) {
            tranPos.turnUsed = turnNo;
            transMove.m = tranPos.bestMove & PackedMove::remove_notYetMoved_mask;
            if (tranPos.depthLeft >= depthleft) {
                //the score in transposition table is stronger than if I let this minimax continue normally
                if (tranPos.flag == EXACT) {
                    return tranPos.score;
                } else if (tranPos.flag == ABOVE_BETA) {
                    if (tranPos.score >= beta)
                        return tranPos.score;
                } else if (tranPos.flag == BELOW_ALPHA) {
                    if (tranPos.score <= alpha)
                        return tranPos.score;
                }
            }

            //unable to return after looking at transposition table
            //check out the move stored in the transposition table
            bitboard.performMove(transMove);
            bitboard.switchSides();
            score_t score = alphaBetaMaxTran(alpha, beta, depthleft - (depth_t) 1, transMove);
            bitboard.switchSides();
            bitboard.undoMove(transMove);
            if (score < bestScore) {
                bestMove = transMove;
                bestScore = score;
            }
            if (score <= alpha) {
                transposition newTrans = {zobristhash, (uint32_t) bestMove, bestScore, BELOW_ALPHA, depthleft, turnNo};
                if (tranPos.shouldOverwrite(newTrans))
                    tranPos = newTrans;
                history.updateHistory(bestMove, depthleft);
                return bestScore;   // fail hard alpha-cutoff
            }
            if (score < beta) {
                beta = score;
            }
            history.addHistoryToMove(transMove);
        }

        auto moves = bitboard.getLegalMoves();
        movesGenerated += moves.length;
        if (moves.empty()) {
            //no moves were created
            if (!(~bitboard.legalMovesToGetOutOfCheck)) {
                //stalemate
                return stalemate;
            } else {
                //checkmate
                return checkmate - (startDepth - depthleft);//encourage checkmate as fast as possible
            }
        }

        std::make_heap(moves.begin(), moves.end());
        while (moves.length) {
            auto &e = moves[0];
            //std::pop_heap(moves.begin(), moves.end());
            //auto &e = moves[--moves.length];
            if (e == transMove) {
                std::pop_heap(moves.begin(), moves.end());
                --moves.length;
                continue;//already looked at this move
            }

            bitboard.performMove(e);
            bitboard.switchSides();
            score_t score = alphaBetaMaxTran(alpha, beta, depthleft - (depth_t) 1, e);
            bitboard.switchSides();
            bitboard.undoMove(e);
            if (score < bestScore) {
                bestMove = e;
                bestScore = score;
            }
            if (score <= alpha) {
                break; // fail hard alpha-cutoff
            }
            if (score < beta) {
                beta = score;
            }
            std::pop_heap(moves.begin(), moves.end());
            --moves.length;
        }

        transposition newTrans = {zobristhash, (uint32_t) bestMove, bestScore, EXACT, depthleft, turnNo};
        if (bestScore >= beta) {
            newTrans.flag = ABOVE_BETA;
        } else if (bestScore <= alpha) {
            newTrans.flag = BELOW_ALPHA;
        }
        if (tranPos.shouldOverwrite(newTrans))
            tranPos = newTrans;

        history.updateHistory(bestMove, depthleft);
        return bestScore;
    }

    score_t alphaBetaMaxQuiescence(score_t alpha, score_t beta, depth_t depthleft, PackedMove lastMove) {
        nodesExplored++;

        if (depthleft == max_quiescence_depth) {
            return evaluate();
        }

        score_t bestScore = -checkmate - 1;
        PackedMove bestMove = {0};
        PackedMove transMove = {0};

        uint32_t zobristhash = (uint32_t) bitboard.zobristHash;
        auto &tranPos = trans.getTransposition(bitboard.zobristHash);

        if (tranPos.hash == zobristhash) {
            tranPos.turnUsed = turnNo;
            transMove.m = tranPos.bestMove & PackedMove::remove_notYetMoved_mask;
            //the score in transposition table is stronger than if I let this minimax continue normally
            if (tranPos.flag == EXACT) {
                return tranPos.score;
            } else if (tranPos.flag == ABOVE_BETA) {
                if (tranPos.score >= beta)
                    return tranPos.score;
            } else if (tranPos.flag == BELOW_ALPHA) {
                if (tranPos.score <= alpha)
                    return tranPos.score;
            }
        }

        score_t standPat = evaluate();
        if (!lastMove.didCheck()) {
            if (standPat >= beta)
                return standPat;

            //todo turn off delta pruning permanently when mtdf manages to reach depth 12
            //delta pruning
            if (lastMove.didPromote()) {
                if (standPat + 900 + 720 < alpha)
                    return standPat + (score_t) 900 + (score_t) 720;
            } else {
                if (standPat + 900 < alpha)
                    return standPat + (score_t) 900;
            }

            if (standPat > alpha)
                alpha = standPat;
        } else {
            if (standPat - 201 >= beta)
                return standPat - (score_t) 201;
        }

        //look at move in transposition table first, before generating other moves
        if (tranPos.hash == zobristhash && (transMove.isNonQuiescence() || lastMove.didCheck())) {
            //unable to return after looking at transposition table
            //check out the move stored in the transposition table
            //todo should uncomment?
            //if (transMove.pieceCaptured() == queens && transMove.pieceMoved() == pawns)
            //    return tranPos.score;
            bitboard.performMove(transMove);
            bitboard.switchSides();
            score_t score = alphaBetaMinQuiescence(alpha, beta, depthleft - (depth_t) 1, transMove);
            bitboard.switchSides();
            bitboard.undoMove(transMove);

            if (score > bestScore) {
                bestMove = transMove;
                bestScore = score;
            }
            if (score >= beta) {
                transposition newTrans = {zobristhash, (uint32_t) bestMove, bestScore, ABOVE_BETA, (depth_t) 0, turnNo};
                if (tranPos.shouldOverwrite(newTrans))
                    tranPos = newTrans;
                //history.updateHistory(bestMove, depthleft);
                return bestScore;   // fail hard beta-cutoff
            }
            if (score > alpha) {
                alpha = score;
            }
            //history.addHistoryToMove(transMove);
        }


        auto moves = bitboard.getLegalMoves();
        movesGenerated += moves.length;
        if (moves.empty()) {
            //no moves were created
            if (!(~bitboard.legalMovesToGetOutOfCheck)) {
                //stalemate
                return stalemate; //todo can't assume stalemate/checkmate like this if I generate only quiescense moves
            } else {
                //checkmate
                return -checkmate + 1;
            }
        }

        std::make_heap(moves.begin(), moves.end());
        while (moves.length) {
            auto &e = moves[0];
            //std::pop_heap(moves.begin(), moves.end());
            //auto &e = moves[--moves.length];
            if (e == transMove) {
                std::pop_heap(moves.begin(), moves.end());
                --moves.length;
                continue;//already looked at this move
            }
            if (!e.isNonQuiescence() && !lastMove.didCheck())
                break; //completed looking at all the captures/promotions/checks, end quiescence search here
            score_t score;
            bitboard.performMove(e);
            bitboard.switchSides();
            //todo should uncomment?
            //if (e.pieceCaptured() == queens && e.pieceMoved() == pawns) {
            //  moves.length = 1;
            //  score = evaluate();
            //} else
            score = alphaBetaMinQuiescence(alpha, beta, depthleft - (depth_t) 1, e);
            bitboard.switchSides();
            bitboard.undoMove(e);

            if (score > bestScore) {
                bestMove = e;
                bestScore = score;
            }
            if (score >= beta) {
                break;
            }
            if (score > alpha) {
                alpha = score;
            }
            std::pop_heap(moves.begin(), moves.end());
            --moves.length;
        }

        if (bestMove == 0) //never looked at a move, therefore completed quiessence searching
            return standPat;

        transposition newTrans = {zobristhash, (uint32_t) bestMove, bestScore, EXACT, (depth_t) 0, turnNo};
        if (bestScore >= beta) {
            newTrans.flag = ABOVE_BETA;
        } else if (bestScore <= alpha) {
            newTrans.flag = BELOW_ALPHA;
        }
        if (tranPos.shouldOverwrite(newTrans))
            tranPos = newTrans;

        bestestMove = bestMove;
        //history.updateHistory(bestMove, depthleft);
        return bestScore;
    }

    score_t alphaBetaMinQuiescence(score_t alpha, score_t beta, depth_t depthleft, PackedMove lastMove) {
        nodesExplored++;

        if (depthleft == max_quiescence_depth) {
            return evaluate();
        }

        score_t bestScore = checkmate + 1;
        PackedMove bestMove = {0};
        PackedMove transMove = {0};

        uint32_t zobristhash = (uint32_t) bitboard.zobristHash;
        auto &tranPos = trans.getTransposition(bitboard.zobristHash);

        if (tranPos.hash == zobristhash) {
            tranPos.turnUsed = turnNo;
            transMove.m = tranPos.bestMove & PackedMove::remove_notYetMoved_mask;
            //the score in transposition table is stronger than if I let this minimax continue normally
            if (tranPos.flag == EXACT) {
                return tranPos.score;
            } else if (tranPos.flag == ABOVE_BETA) {
                if (tranPos.score >= beta)
                    return tranPos.score;
            } else if (tranPos.flag == BELOW_ALPHA) {
                if (tranPos.score <= alpha)
                    return tranPos.score;
            }
        }

        score_t standPat = evaluate();
        if (!lastMove.didCheck()) {
            //score_t standPat = evaluate();
            if (standPat <= alpha)
                return standPat;

            //delta pruning
            if (lastMove.didPromote()) {
                if (standPat - 900 - 720 > beta)
                    return standPat - (score_t) 900 - (score_t) 720;
            } else {
                if (standPat - 900 > beta)
                    return standPat - (score_t) 900;
            }

            if (standPat < beta)
                beta = standPat;
        } else {
            //score_t standPat = evaluate();
            if (standPat + 201 <= alpha)
                return standPat + (score_t) 201;
        }

        if (tranPos.hash == zobristhash && (transMove.isNonQuiescence() || lastMove.didCheck())) {
            //unable to return after looking at transposition table
            //check out the move stored in the transposition table
            //todo should uncomment?
            //if (transMove.pieceCaptured() == queens && transMove.pieceMoved() == pawns)
            //    return tranPos.score;

            bitboard.performMove(transMove);
            bitboard.switchSides();
            score_t score = alphaBetaMaxQuiescence(alpha, beta, depthleft - (depth_t) 1, transMove);
            bitboard.switchSides();
            bitboard.undoMove(transMove);
            if (score < bestScore) {
                bestMove = transMove;
                bestScore = score;
            }
            if (score <= alpha) {
                transposition newTrans = {zobristhash, (uint32_t) bestMove, bestScore, BELOW_ALPHA, (depth_t) 0, turnNo};
                if (tranPos.shouldOverwrite(newTrans))
                    tranPos = newTrans;
                //history.updateHistory(bestMove, depthleft);
                return bestScore;   // fail hard alpha-cutoff
            }
            if (score < beta) {
                beta = score;
            }
            //history.addHistoryToMove(transMove);
        }

        auto moves = bitboard.getLegalMoves();
        movesGenerated += moves.length;
        if (moves.empty()) {
            //no moves were created
            if (!(~bitboard.legalMovesToGetOutOfCheck)) {
                //stalemate
                return stalemate;//todo can't assume stalemate/checkmate like this if I generate only quiescense moves
            } else {
                //checkmate
                return checkmate - (startDepth - depthleft);//encourage checkmate as fast as possible
            }
        }

        std::make_heap(moves.begin(), moves.end());
        while (moves.length) {
            auto &e = moves[0];
            //std::pop_heap(moves.begin(), moves.end());
            //auto &e = moves[--moves.length];
            if (e == transMove) {
                std::pop_heap(moves.begin(), moves.end());
                --moves.length;
                continue;//already looked at this move
            }
            if (!e.isNonQuiescence() && !lastMove.didCheck())
                break; //completed looking at all the captures/promotions/checks, end quiescence search here

            score_t score;
            bitboard.performMove(e);
            bitboard.switchSides();
            //todo should uncomment?
            //if (e.pieceCaptured() == queens && e.pieceMoved() == pawns) {
            //    moves.length = 1;
            //    score = evaluate();
            //} else
            score = alphaBetaMaxQuiescence(alpha, beta, depthleft - (depth_t) 1, e);
            bitboard.switchSides();
            bitboard.undoMove(e);
            if (score < bestScore) {
                bestMove = e;
                bestScore = score;
            }
            if (score <= alpha) {
                break; // fail hard alpha-cutoff
            }
            if (score < beta) {
                beta = score;
            }
            std::pop_heap(moves.begin(), moves.end());
            --moves.length;
        }

        if (bestMove == 0) //never looked at a move, therefore completed quiessence searching
            return standPat;

        transposition newTrans = {zobristhash, (uint32_t) bestMove, bestScore, EXACT, (depth_t) 0, turnNo};
        if (bestScore >= beta) {
            newTrans.flag = ABOVE_BETA;
        } else if (bestScore <= alpha) {
            newTrans.flag = BELOW_ALPHA;
        }
        if (tranPos.shouldOverwrite(newTrans))
            tranPos = newTrans;

        //history.updateHistory(bestMove, depthleft);
        return bestScore;
    }

    score_t alphaBetaMax(score_t alpha, score_t beta, depth_t depthleft) {
        nodesExplored++;
        if (depthleft == 0)
            return evaluate();

        auto moves = bitboard.getLegalMoves();
        movesGenerated += moves.length;
        if (moves.empty()) {
            //no moves were created
            if (!(~bitboard.legalMovesToGetOutOfCheck)) {
                //stalemate
                return stalemate;
            } else {
                //checkmate
                return -checkmate;
            }
        }
        auto bestMove = moves[0];
        std::make_heap(moves.begin(), moves.end());
        while (moves.length) {
            std::pop_heap(moves.begin(), moves.end());
            auto &e = moves[--moves.length];
            bitboard.performMove(e);
            score_t score;
            score = alphaBetaMin(alpha, beta, depthleft - (depth_t) 1);
            bitboard.undoMove(e);
            if (score >= beta)
                return beta;   // fail hard beta-cutoff
            if (score > alpha) {
                bestMove = e;
                alpha = score; // alpha acts like max in MiniMax
            }
        }
        bestestMove = bestMove;
        return alpha;
    }

    score_t alphaBetaMin(score_t alpha, score_t beta, depth_t depthleft) {
        nodesExplored++;
        if (depthleft == 0)
            return evaluate();

        bitboard.switchSides();
        auto moves = bitboard.getLegalMoves();
        movesGenerated += moves.length;
        if (moves.empty()) {
            //no moves were created
            if (!(~bitboard.legalMovesToGetOutOfCheck)) {
                //stalemate
                //long debugging to catch my not switching sides here,
                //    could probably wrap switch sides into a scoped destruction to handle all exit paths
                bitboard.switchSides();
                return stalemate;
            } else {
                //checkmate
                bitboard.switchSides();
                return checkmate - (startDepth - depthleft);//encourage checkmate as fast as possible
            }
        }
        std::make_heap(moves.begin(), moves.end());
        while (moves.length) {
            std::pop_heap(moves.begin(), moves.end());
            auto &e = moves[--moves.length];
            bitboard.performMove(e);
            bitboard.switchSides();
            score_t score = alphaBetaMax(alpha, beta, depthleft - (depth_t) 1);
            bitboard.switchSides();
            bitboard.undoMove(e);
            if (score <= alpha) {
                bitboard.switchSides();
                return alpha; // fail hard alpha-cutoff
            }
            if (score < beta)
                beta = score; // beta acts like min in MiniMax
        }
        bitboard.switchSides();
        return beta;
    }


}