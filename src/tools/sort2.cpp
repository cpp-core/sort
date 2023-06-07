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

template<class Iter>
void bitonic_split(Iter begin, Iter end) {
    auto left_iter = begin;
    auto right_iter = begin + (end - begin) / 2;
    while (right_iter < end and *left_iter < *right_iter) {
	++left_iter;
	++right_iter;
    }
    
    if (right_iter < end) {
	size_t n = left_iter - begin;
	std::swap(left_iter, left_iter + n, left_iter + n);
    }
}


int main(int argc, const char *argv[]) {
    size_t nrecords = argc < 2 ? (32 * 1024 * 1024) : atoi(argv[1]);
    
    std::uniform_int_distribution<uint64_t> d;
    std::mt19937_64 rng;
    std::vector<uint64_t> data(nrecords);
    std::generate(data.begin(), data.end(), [&]() { return d(rng); });

    timer::Timer timer;
    timer.start();

    std::sort(data.begin(), data.begin() + nrecords / 2);
    std::sort(data.begin() + nrecords / 2, data.end());

    auto left = &data[0], right = &data.back(), middle = &data[0] + nrecords / 2;
    while (left < right and *left < *right) {
	++left;
	--right;
    }
    std::swap_ranges(left, middle, middle);

    // auto left_max = std::accumulate(&data[0], middle, uint64_t{},
    // 				    [](auto a, auto b) { return std::max(a, b); });
    // auto right_min = std::accumulate(middle, &data[nrecords], uint64_t(-1),
    // 				     [](auto a, auto b) { return std::min(a, b); });

    timer.stop();
    // cout << left_max << endl;
    // cout << right_min << endl;    
    cout << (1e-9 * timer.elapsed().count()) << endl;
    
    // for (auto i = 1; i < data.size(); ++i)
    // 	if (data[i-1] > data[i])
    // 	    cout << i << " " << data[i-1] << " " << data[i]  << endl;
    
    return 0;
}
