// Copyright (C) 2022, 2023 by Mark Melton
//

#include <algorithm>
#include <iostream>
#include <queue>
#include <random>
#include <thread>
#include <barrier>
#include "core/timer/timer.h"

using std::cout, std::endl;
using namespace core;

template<class T, class Compare>
void bitonic_sort(T *data, size_t n, Compare compare) {
    for (auto k = 2; k <= n; k *= 2) {
	for (auto j = k/2; j > 0; j /= 2) {
	    for (auto i = 0; i < n; ++i) {
		auto l = i xor j;
		if (l > i) {
		    bool mask = i bitand k;
		    bool cmp = compare(data[i], data[l]);
		    if (not (mask ^ cmp))
			std::swap(data[i], data[l]);
		}
	    }
	}
    }
}


int main(int argc, const char *argv[]) {
    // size_t nth = argc < 2 ? 2 : atoi(argv[1]);
    size_t nrecords = argc < 3 ? 100'000'000 : atoi(argv[2]);
    
    std::uniform_int_distribution<uint64_t> d;
    std::mt19937_64 rng;
    std::vector<uint64_t> data(nrecords);
    std::generate(data.begin(), data.end(), [&]() { return d(rng); });

    timer::Timer timer;
    timer.start();

    bitonic_sort(&data[0], data.size(), std::less{});

    // auto begin = data.begin(), end = data.end();
    // size_t ndata = end - begin;
    // size_t bucketsize = ndata / nth;

    // std::barrier sync(nth);
    // std::vector<std::thread> threads;
    // for (auto tid = 0; tid < nth; ++tid) {
    // 	threads.emplace_back([&,tid]() {
    // 	    size_t sdx = tid * bucketsize;
    // 	    size_t edx = std::min(sdx + bucketsize, ndata);
    // 	    std::sort(begin + sdx, begin + edx);
    // 	});
    // }

    // for (auto& thread : threads)
    // 	if (thread.joinable())
    // 	    thread.join();

    timer.stop();
    
    cout << (1e-9 * timer.elapsed().count()) << endl;
    
    for (auto i = 1; i < data.size(); ++i)
	if (data[i-1] > data[i])
	    cout << i << " " << data[i-1] << " " << data[i]  << endl;
    
    return 0;
}
