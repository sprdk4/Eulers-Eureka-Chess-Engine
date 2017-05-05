//
// Created by Shawn Roach on 4/14/2017.
//

#ifndef CPP_CLIENT_PONDERING_H
#define CPP_CLIENT_PONDERING_H


namespace BitBoard {

//used as macro gaurd to prevent inability to compile std::thread
//#define usePondering

    void startPonderingThread();

    void endPonderingThread();

    void ponderingRoutine();

}
#endif //CPP_CLIENT_PONDERING_H
