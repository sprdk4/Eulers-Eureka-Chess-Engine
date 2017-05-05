//
// Created by Shawn Roach on 11/11/2016.
// on 11/30/2016, I Shawn Roach, as the author of this file, officialy declare it to be placed in public domain.
//


#include <iostream>
#include <sstream>
#include "ScopedBenchmark.h"

typedef std::chrono::nanoseconds resolution;
const std::string resolutionSuffix = " ns";

std::map<std::string, ScopedBenchmark::benchmark> ScopedBenchmark::marks;

ScopedBenchmark::ScopedBenchmark(const std::string key) : start(std::chrono::high_resolution_clock::now()), key(key) {}

ScopedBenchmark::~ScopedBenchmark() {
    auto now = std::chrono::high_resolution_clock::now();
    auto &e = marks[key];
    e.numBenches++;
    e.totalElapsedTime += std::chrono::duration_cast<resolution>(now - start).count();
}

//fix issue with mingw that claims to_string doesn't exist in std::
namespace std {
    template<typename T>
    std::string to_string(const T &n) {
        std::ostringstream stm;
        stm << n;
        return stm.str();
    }
}

template<typename T>
std::string addDigitGrouping(T avg) {
    std::string AvgWithCommas = std::to_string(avg);
    int insertPosition = AvgWithCommas.length() - 3;
    while (insertPosition > 0) {
        AvgWithCommas.insert((unsigned) insertPosition, ",");
        insertPosition -= 3;
    }
    return AvgWithCommas;
}

void adjustRight(std::string &str, unsigned toLength) {
    std::string blanks;
    blanks.resize(toLength - str.length(), ' ');
    str.insert(0, blanks);
}

void ScopedBenchmark::printBenchMarks() {
    if (marks.empty())
        return;//nothing to print out

    unsigned longestKey = 0;
    unsigned longestAvgNumDigits = 0;
    unsigned longestTotalNumDigits = 0;
    for (auto it = marks.begin(); it != marks.end(); ++it) {
        std::string key = it->first;
        if (longestKey < key.size())
            longestKey = key.size();

        auto &e = it->second;
        auto avg = e.totalElapsedTime / e.numBenches;

        std::string AvgWithCommas = addDigitGrouping(avg);
        if (longestAvgNumDigits < AvgWithCommas.size())
            longestAvgNumDigits = AvgWithCommas.size();

        std::string TotalWithCommas = addDigitGrouping(e.totalElapsedTime);
        if (longestTotalNumDigits < TotalWithCommas.size())
            longestTotalNumDigits = TotalWithCommas.size();
    }
    std::cout << "Scoped Benchmarking results:\n";
    for (auto it = marks.begin(); it != marks.end(); ++it) {
        std::string key = it->first;
        auto &e = it->second;
        auto avg = e.totalElapsedTime / e.numBenches;
        std::string AvgWithCommas = addDigitGrouping(avg) + resolutionSuffix;
        std::string TotalWithCommas = addDigitGrouping(e.totalElapsedTime) + resolutionSuffix;

        adjustRight(AvgWithCommas, longestAvgNumDigits + resolutionSuffix.size());
        adjustRight(TotalWithCommas, longestTotalNumDigits + resolutionSuffix.size());
        key.resize(longestKey, ' ');

        std::cout << key << " average: " << AvgWithCommas << "  total: " << TotalWithCommas << "  Samples: "
                  << e.numBenches << '\n';
    }
    std::cout << "note: averages below 5,000 ns can be fuzzy as the benchmark finished too fast\n";
}
