// Copyright (C) 2022, 2023 by Mark Melton
//

#include <iostream>
#include <random>
#include "core/timer/timer.h"
#include "core/sort/psort_merge.h"

using std::cout, std::endl;

int main(int argc, const char *argv[]) {
    size_t nth = argc < 2 ? 2 : atoi(argv[1]);
    size_t nrecords = argc < 3 ? 100'000'000 : atoi(argv[2]);
    
    if (std::popcount(nth) != 1) {
	cout << "Number of threads must be power of two: " << nth << endl;
	return -1;
    }
    
    std::vector<uint64_t> data(nrecords);
    std::uniform_int_distribution<uint64_t> d;
    std::mt19937_64 rng;
    std::generate(data.begin(), data.end(), [&]() { return d(rng); });

    core::timer::Timer timer;
    timer.start();
    
    core::sort::psort_merge(nth, data.begin(), data.end(), std::less{});
    
    timer.stop();
    cout << (1e-9 * timer.elapsed().count()) << endl;

    for (auto i = 1; i < data.size(); ++i)
	if (data[i-1] > data[i])
	    cout << i << " " << data[i-1] << " " << data[i]  << endl;

    return 0;
}
