// Copyright (C) 2022, 2023 by Mark Melton
//

#include <algorithm>
#include <array>
#include <iostream>
#include <random>
#include "core/timer/timer.h"

using std::cout, std::endl;
using namespace core;

namespace core::sort {

#define REPEAT4(code) code; code; code; code
#define REPEAT8(code) REPEAT4(code); REPEAT4(code)

inline constexpr auto BlockSize = 64;
inline constexpr auto CacheLineSize = 64;

template<class Iter, class Compare>
void bitonic_merge(Iter begin, Iter end, Compare compare) {
    auto mdx = (end - begin) / 2;
    while (mdx > 8) {
	auto middle = begin + mdx;
	auto i1 = begin, i2 = middle;
	while (i1 < middle) {
	    while (i1 < middle and compare(*i1, *i2)) {
		++i1;
		++i2;
	    }
	    auto j1 = i1, j2 = i2;
	    while (i1 < middle and not compare(*i1, *i2)) {
		++i1;
		++i2;
	    }
	    std::swap_ranges(j1, i1, j2);
	}
	
	bitonic_merge(begin, middle, compare);
	begin = middle;
	mdx = (end - begin) / 2;
    }

    if (mdx == 8) {
	for (auto iter = begin; iter < end; iter += 16) {
	    for (auto i = 0; i < 8; ++i)
		if (not compare(*(iter + i), *(iter + i + 8)))
		    std::iter_swap(iter + i, iter + i + 8);
	}
	mdx /= 2;
    }
    
    if (mdx == 4) {
	for (auto iter = begin; iter < end; iter += 8) {
	    if (not compare(*iter, *(iter + 4)))
		std::iter_swap(iter, iter + 4);
	    if (not compare(*(iter + 1), *(iter + 5)))
		std::iter_swap(iter + 1, iter + 5);
	    if (not compare(*(iter + 2), *(iter + 6)))
		std::iter_swap(iter + 2, iter + 6);
	    if (not compare(*(iter + 3), *(iter + 7)))
		std::iter_swap(iter + 3, iter + 7);

	    if (not compare(*iter, *(iter + 2)))
		std::iter_swap(iter, iter + 2);
	    if (not compare(*(iter + 1), *(iter + 3)))
		std::iter_swap(iter + 1, iter + 3);
	    if (not compare(*(iter + 4), *(iter + 6)))
		std::iter_swap(iter + 4, iter + 6);
	    if (not compare(*(iter + 5), *(iter + 7)))
		std::iter_swap(iter + 5, iter + 7);

	    if (not compare(*(iter), *(iter + 1)))
		std::iter_swap(iter, iter + 1);
	    if (not compare(*(iter + 2), *(iter + 3)))
		std::iter_swap(iter + 2, iter + 3);
	    if (not compare(*(iter + 4), *(iter + 5)))
		std::iter_swap(iter + 4, iter + 5);
	    if (not compare(*(iter + 6), *(iter + 7)))
		std::iter_swap(iter + 6, iter + 7);
	}
    } else if (mdx == 2) {
	for (auto iter = begin; iter < end; iter += 4) {
	    if (not compare(*iter, *(iter + 2)))
		std::iter_swap(iter, iter + 2);
	    if (not compare(*(iter + 1), *(iter + 3)))
		std::iter_swap(iter + 1, iter + 3);

	    if (not compare(*(iter), *(iter + 1)))
		std::iter_swap(iter, iter + 1);
	    if (not compare(*(iter + 2), *(iter + 3)))
		std::iter_swap(iter + 2, iter + 3);
	}
    } else if (mdx == 1) {
	for (auto iter = begin; iter < end; iter += 2) {
	    if (not compare(*(iter), *(iter + 1)))
		std::iter_swap(iter, iter + 1);
	}
    }
}

template<class Iter, class Compare>
void bitonic(Iter begin, Iter end, Compare compare) {
    // std::sort(begin, end);
    auto mdx = (end - begin) / 2;
    auto middle = begin + mdx;
    std::sort(begin, middle, compare);
    std::sort(middle, end, compare);
    std::reverse(middle, end);
    bitonic_merge(begin, end, compare);
}

};

int main(int argc, const char *argv[]) {
    // size_t nworkers = argc < 2 ? 2 : atoi(argv[1]);
    size_t nrecords = argc < 2 ? (16 * 1024 * 1024) : atoi(argv[1]);
    
    std::uniform_int_distribution<uint64_t> d;
    std::mt19937_64 rng;
    std::vector<uint64_t> data(nrecords);
    std::generate(data.begin(), data.end(), [&]() { return d(rng); });

    timer::Timer timer;
    timer.start();
    sort::bitonic(data.begin(), data.end(), std::less{});
    timer.stop();
    cout << (1e-9 * timer.elapsed().count()) << endl;

    for (auto i = 1; i < data.size(); ++i)
	if (data[i-1] > data[i])
	    cout << "not sorted : " << i << " " << data[i-1] << " " << data[i]  << endl;
    
    return 0;
}

