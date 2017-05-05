//
// Created by Shawn Roach on 4/12/2017.
//

#ifndef CPP_CLIENT_EVALUATION_H
#define CPP_CLIENT_EVALUATION_H

#include "Move.h"

namespace BitBoard {
    typedef int16_t score_t;
    constexpr score_t checkmate = 20000;
    constexpr score_t stalemate = -1001;

    // expected assertion is that:
    // fast_evaluate() - max_negative_fast_eval_diff_from_eval <= evaluate()
    // evaluate() <= fast_evaluate() + max_positive_fast_eval_diff_from_eval

    // the greatest positive differences would be caused by a large number of distantl passed pawns,
    //constexpr score_t max_positive_fast_eval_diff_from_eval=89;//monte carlo simulation computed this to be atleast 68,
    //constexpr score_t max_negative_fast_eval_diff_from_eval=290;//monte carlo simulation computed this to be atleast 448 (well fuck!)

    score_t evaluate();

    score_t fast_evaluate();
}
#endif //CPP_CLIENT_EVALUATION_H
