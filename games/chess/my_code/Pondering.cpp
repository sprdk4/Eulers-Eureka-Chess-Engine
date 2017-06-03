//
// Created by Shawn Roach on 4/14/2017.
//

#include "Pondering.h"

#ifdef usePondering

#include <thread>

#endif

#include <unordered_set>
#include <algorithm>
#include <iostream>
#include "MoveSelection.h"
#include "TranspositionTable.h"
#include "BitBoard.h"
#include "HistoryTable.h"
#include "ScopedBenchmark.h"

extern BitBoard::BitBoard bitboard;
namespace BitBoard {
#ifdef usePondering
    std::thread ponderingThread;
#endif

    extern TranspositionTable trans;
    extern std::unordered_set<u64> prevStates;
    extern depth_t startDepth;
    extern uint8_t turnNo;

    bool continueToPonder = false;

    score_t alphaBetaMaxTranPonder(score_t alpha, score_t beta, depth_t depthleft, PackedMove lastMove);

    score_t alphaBetaMinTranPonder(score_t alpha, score_t beta, depth_t depthleft, PackedMove lastMove);

    score_t alphaBetaMaxTranPonder(score_t alpha, score_t beta, depth_t depthleft, PackedMove lastMove) {

        if (depthleft == 0) {
            return alphaBetaMaxQuiescence(alpha, beta, depthleft, lastMove);
        }

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
                    return tranPos.score;
                } else if (tranPos.flag == ABOVE_BETA) {
                    if (tranPos.score >= beta) {
                        return tranPos.score;
                    }
                } else if (tranPos.flag == BELOW_ALPHA) {
                    if (tranPos.score <= alpha) {
                        return tranPos.score;
                    }
                }
            }

            //unable to return after looking at transposition table
            //check out the move stored in the transposition table
            bitboard.performMove(transMove);
            score_t score;
            if (depthleft >= startDepth - 2 && transMove < PackedMove::double_pawn_forward_flag &&
                prevStates.find(bitboard.zobristHash) != prevStates.end()) {
                score = stalemate;
            } else {
                bitboard.switchSides();
                score = alphaBetaMinTranPonder(alpha, beta, depthleft - (depth_t) 1, transMove);
                bitboard.switchSides();
            }
            bitboard.undoMove(transMove);
            if (!continueToPonder)
                return checkmate + 100;//identifiable invalid
            if (score > bestScore) {
                bestMove = transMove;
                bestScore = score;
            }
            if (score >= beta) {
                transposition newTrans = {zobristhash, (uint32_t) bestMove, bestScore, ABOVE_BETA, depthleft, turnNo};
                if (tranPos.shouldOverwrite(newTrans))
                    tranPos = newTrans;
                history.updateHistory(bestMove, depthleft);
                return bestScore;   // fail hard beta-cutoff
            }
            if (score > alpha) {
                alpha = score;
            }
            history.addHistoryToMove(transMove);
        }

        auto moves = bitboard.getLegalMoves();
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
            std::pop_heap(moves.begin(), moves.end());
            auto &e = moves[--moves.length];
            if (e == transMove)
                continue;//already looked at this move
            bitboard.performMove(e);
            score_t score;
            if (depthleft >= startDepth - 2 && e < PackedMove::double_pawn_forward_flag && prevStates.size() > 0 &&
                prevStates.find(bitboard.zobristHash) != prevStates.end()) {
                // this is the root of the tree, and I am about to explore a move that has already been generated previously in the game
                // distinctive emphasis: previously in the game, not previously in exploration
                // this prevents three-fold repetition.
                score = stalemate;
            } else {
                bitboard.switchSides();
                score = alphaBetaMinTranPonder(alpha, beta, depthleft - (depth_t) 1, e);
                bitboard.switchSides();
            }
            bitboard.undoMove(e);
            if (!continueToPonder)
                return checkmate + 100;//identifiable invalid

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

    score_t alphaBetaMinTranPonder(score_t alpha, score_t beta, depth_t depthleft, PackedMove lastMove) {
        if (depthleft == 0) {
            return alphaBetaMinQuiescence(alpha, beta, depthleft, lastMove);
        }

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
            score_t score = alphaBetaMaxTranPonder(alpha, beta, depthleft - (depth_t) 1, transMove);
            bitboard.switchSides();
            bitboard.undoMove(transMove);
            if (!continueToPonder)
                return checkmate + 100;//identifiable invalid
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
            std::pop_heap(moves.begin(), moves.end());
            auto &e = moves[--moves.length];
            if (e == transMove)
                continue;//already looked at this move

            bitboard.performMove(e);
            bitboard.switchSides();
            score_t score = alphaBetaMaxTranPonder(alpha, beta, depthleft - (depth_t) 1, e);
            bitboard.switchSides();
            bitboard.undoMove(e);
            if (!continueToPonder)
                return checkmate + 100;//identifiable invalid
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


    void startPonderingThread() {
#ifdef usePondering
        //std::cout << "starting pondering thread\n";
        continueToPonder = true;
        startDepth = 0;
        try {
            ponderingThread = std::thread(ponderingRoutine);
        } catch (std::system_error e) {
            std::cout << "encountered system_error in startPonderingThread()\n";
            std::cout << "Caught system_error with code " << e.code()
                      << " meaning " << e.what() << '\n';
            std::cout << std::flush;
            throw e;
        }
#endif
    }

    void endPonderingThread() {
#ifdef usePondering
        //std::cout << "ending pondering thread\n";
        continueToPonder = false;
        try {
            if (ponderingThread.joinable())
                ponderingThread.join();
        } catch (std::system_error e) {
            std::cout << "encountered system_error in endPonderingThread()\n";
            std::cout << "Caught system_error with code " << e.code()
                      << " meaning " << e.what() << '\n';
            std::cout << std::flush;
            throw e;
        }
#endif
    }

    score_t mtdfPonder(score_t g, depth_t depthLeft) {
        //http://people.csail.mit.edu/plaat/mtdf.html#abmem
        score_t upper = std::numeric_limits<score_t>::max();
        score_t lower = std::numeric_limits<score_t>::lowest();
        score_t beta;
        while (lower < upper) {
            if (g == lower)
                beta = g + (score_t) 1;
            else
                beta = g;
            g = alphaBetaMaxTranPonder(beta - (score_t) 1, beta, depthLeft, {0});
            if (g < beta)
                upper = g;
            else
                lower = g;
        }
        return g;
    }

    void ponderingRoutine() {
        SCOPED_BENCHMARK("pondering");
        auto start = std::chrono::high_resolution_clock::now();
        bitboard.switchSides();
        turnNo++;
        score_t min = checkmate;
        //score_t guesses[218];
        for (depth_t onDepth = 1; continueToPonder; onDepth++) {
            //todo for every iteration, sort the moves by their values from previous iteration, to increase chances of searching the move that the opponent will pick first.
            auto moves = bitboard.getLegalMoves();
            for (int x = 0; x < moves.length; x++) {
                auto &e = moves[x];
                bitboard.performMove(e);
                score_t score;
                if (prevStates.find(bitboard.zobristHash) != prevStates.end()) {
                    score = stalemate;
                } else {
                    bitboard.switchSides();
                    if (prevStates.find(bitboard.zobristHash) != prevStates.end()) {
                        score = stalemate; //todo to lazy to figure out if I needed to switch sides or not in this check
                    } else {
                        //score_t score = mtdfPonder(guesses[x], startDepth = onDepth);
                        //guesses[x]=score;
                        //using vanilla alphabeta instead of mtdf to get a better coverage of the tree from each branch, and to fill the
                        //transposition table up with exact nodes, which mtdf doesn't do.
                        score = alphaBetaMaxTranPonder(std::numeric_limits<score_t>::lowest(), std::numeric_limits<score_t>::max(),
                                                       startDepth = onDepth, {0});
                    }
                    bitboard.switchSides();
                }
                bitboard.undoMove(e);
                min = std::min(min, score);
                if (!continueToPonder)
                    break;
            }
            if (!(min < checkmate - onDepth && min > -checkmate)) {//garunteed checkmate, don't waste useless time
                break;
            }
        }
        bitboard.switchSides();
        auto timeTaken = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count();
        std::cout << "pondered for: " << timeTaken << " ns\n";
        std::cout << "pondered to depth: " << (int) startDepth << '\n';
    }

}//end namespace bitboard
