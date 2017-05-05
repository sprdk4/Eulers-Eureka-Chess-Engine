//
// Created by Shawn Roach on 11/11/2016.
// on 11/30/2016, I Shawn Roach, as the author of this file, officialy declare it to be placed in public domain.
//

#ifndef INC_2016FS_A_2B_SPRDK4_SCOPEDBENCHMARK_H
#define INC_2016FS_A_2B_SPRDK4_SCOPEDBENCHMARK_H

#include <string>
#include <chrono>
#include <map>


//to benchmark simply call SCOPED_BENCHMARK(<string identifier for scope>) at the beginning of the scope you want to benchmark
//to see benchmarks simply call ScopedBenchmark::printBenchMarks();
class ScopedBenchmark {
private:
    typedef struct {
        uint64_t numBenches = 0;
        uint64_t totalElapsedTime = 0;
    } benchmark;
    static std::map<std::string, benchmark> marks;
public:
    std::chrono::high_resolution_clock::time_point start;
    std::string key;

    ScopedBenchmark(const std::string key);

    ~ScopedBenchmark();

    static void printBenchMarks();
};


#define TokenPasting__2(a, b, c) a##b
#define TokenPasting__1(a, b, c) TokenPasting__2(a,b,c)
#define BenchExpand__(line, function) TokenPasting__1(timer__,line,function)
//#ifndef NDEBUG
#define SCOPED_BENCHMARK __attribute((unused)) ScopedBenchmark BenchExpand__(__LINE__,__FUNCTION__)
//#else
//#define SCOPED_BENCHMARK(...)
//#endif
#endif //INC_2016FS_A_2B_SPRDK4_SCOPEDBENCHMARK_H