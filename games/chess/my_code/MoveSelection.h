//
// Created by Shawn Roach on 3/14/2017.
//

#ifndef CPP_CLIENT_MOVESELECTION_H
#define CPP_CLIENT_MOVESELECTION_H

#include "Move.h"
#include "Evaluation.h"

namespace BitBoard {

    typedef int8_t depth_t;

    constexpr depth_t max_quiescence_depth = -10; //assumably will quiescence will never search this far down

    score_t evaluate();

    score_t alphaBetaMax(score_t alpha, score_t beta, depth_t depthleft);

    score_t alphaBetaMin(score_t alpha, score_t beta, depth_t depthleft);

    score_t alphaBetaMaxTran(score_t alpha, score_t beta, depth_t depthleft, PackedMove lastMove);

    score_t alphaBetaMinTran(score_t alpha, score_t beta, depth_t depthleft, PackedMove lastMove);

    score_t alphaBetaMaxQuiescence(score_t alpha, score_t beta, depth_t depthleft, PackedMove lastMove);

    score_t alphaBetaMinQuiescence(score_t alpha, score_t beta, depth_t depthleft, PackedMove lastMove);

    PackedMove getBestMove(const double& time_remaining);

    void init();
}

#endif //CPP_CLIENT_MOVESELECTION_H
