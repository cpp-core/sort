// Copyright (C) 2022, 2023 by Mark Melton
//

#include <iostream>
#include <random>
#include "core/timer/timer.h"
#include "core/record/iterator.h"

using std::cout, std::endl;
using namespace core;

int main(int argc, const char *argv[]) {
    // size_t nworkers = argc < 2 ? 2 : atoi(argv[1]);
    size_t nrecords = argc < 3 ? 100'000'000 : atoi(argv[2]);
    
    std::uniform_int_distribution<uint64_t> d;
    std::mt19937_64 rng;
    std::vector<uint64_t> data(nrecords);
    std::generate(data.begin(), data.end(), [&]() { return d(rng); });

    timer::Timer timer;
    timer.start();
    std::sort(data.begin(), data.end());
    timer.stop();
    
    cout << (1e-9 * timer.elapsed().count()) << endl;

    for (auto i = 1; i < data.size(); ++i)
	if (data[i-1] > data[i])
	    cout << i << " " << data[i-1] << " " << data[i]  << endl;
    
    return 0;
}
