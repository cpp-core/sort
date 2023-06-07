// Copyright (C) 2022, 2023 by Mark Melton
//

#undef NDEBUG
#include <cassert>

#include <iostream>
#include <random>
#include "core/timer/timer.h"

#include <algorithm>
#include <array>
namespace core::sort {

inline namespace qsort_detail {
       inline constexpr size_t InsertionSortThreshold = 24;
       inline constexpr size_t PseudoMedianThreshold = 1024 * 1024 * 1024;
       inline constexpr size_t CacheLineSize = 64;
       inline constexpr size_t BlockSize = 64;
};

template<class Iter, class Compare>
void sort_elements(Iter a, Iter b, Compare compare) {
    if (compare(*b, *a))
	std::iter_swap(a, b);
}

template<class Iter, class Compare>
void sort_elements(Iter a, Iter b, Iter c, Compare compare) {
    sort_elements(a, b, compare);
    sort_elements(b, c, compare);
    sort_elements(a, b, compare);
}

template<class Iter, class Compare>
void move_pivot_to_begin(Iter begin, Iter end, Compare compare) {
    auto size = end - begin;
    auto mdx = size / 2;
    if (size > PseudoMedianThreshold) {
	sort_elements(begin, begin + mdx, end - 1, compare);
	sort_elements(begin + 1, begin + mdx - 1, end - 2, compare);
	sort_elements(begin + 2, begin + mdx + 1, end - 3, compare);
	sort_elements(begin + mdx - 1, begin + mdx, begin + mdx + 1, compare);
	std::iter_swap(begin, begin + mdx);
    } else {
	sort_elements(begin + mdx, begin, end - 1, compare);
    }
}

template<class Iter, class Compare>
void insertion_sort(Iter begin, Iter end, Compare compare) {
    using T = typename Iter::value_type;

    if (end - begin < 2)
	return;

    for (auto iter = begin + 1; iter < end; ++iter) {
	auto a = iter, b = iter - 1;
	if (compare(*a, *b)) {
	    T tmp = std::move(*a);

	    do {
		*a-- = std::move(*b);
	    } while (a != begin and compare(tmp, *--b));

	    *a = std::move(tmp);
	}
    }
}

template<class Iter, class Compare>
auto partition_basic(Iter begin, Iter end, Compare compare) {
    using std::swap;
    using T = typename Iter::value_type;
    
    T pivot = std::move(*begin);
    Iter left = begin;
    Iter right = end;

    while (compare(*++left, pivot));
    while (left < right and not compare(*--right, pivot));

    while (left < right) {
	swap(*left, *right);
	while (compare(*++left, pivot));
	while (not compare(*--right, pivot));
    }

    Iter piter = left - 1;
    *begin = std::move(*piter);
    *piter = std::move(pivot);

    return piter;
}

template<class Iter, class Compare>
auto partition(Iter begin, Iter end, Compare compare) {
    using std::swap;
    using T = typename Iter::value_type;
    
    T pivot = std::move(*begin);
    Iter left = begin + 1;
    Iter right = end;

    alignas(CacheLineSize) std::array<unsigned char, BlockSize> left_offsets;
    alignas(CacheLineSize) std::array<unsigned char, BlockSize> right_offsets;
    while (right - left > 2 * BlockSize) {
	Iter l = left;
	size_t ldx{};
	for (auto i = 0; i < BlockSize; ++i) {
	    left_offsets[ldx] = i;
	    ldx += not compare(*l++, pivot);
	}

	Iter r = right;
	size_t rdx{};
	for (auto i = 0; i < BlockSize; ++i) {
	    right_offsets[rdx] = i;
	    rdx += compare(*--r, pivot);
	}

	auto n = std::min(ldx, rdx);
	for (auto i = 0; i < n; ++i)
	    swap(left[left_offsets[i]], right[-right_offsets[i] - 1]);

	ldx -= n;
	rdx -= n;

	left += (ldx == 0 ? BlockSize : left_offsets[n]);
	right -= (rdx == 0 ? BlockSize : right_offsets[n]);
    }

    if (left < right) {
	while (compare(*left, pivot))
	    ++left;
	while (left < right and not compare(*--right, pivot));
	
	while (left < right) {
	    swap(*left, *right);
	    while (compare(*++left, pivot));
	    while (not compare(*--right, pivot));
	}
    }

    Iter piter = left - 1;
    *begin = std::move(*piter);
    *piter = std::move(pivot);

    return piter;
}

template<class Iter, class Compare>
void qsort(Iter begin, Iter end, Compare compare) {
    while (true) {
	auto size = end - begin;

	if (size < InsertionSortThreshold) {
	    insertion_sort(begin, end, compare);
	    return;
	}

	move_pivot_to_begin(begin, end, compare);
	auto piter = core::sort::partition(begin, end, compare);
	
	qsort(begin, piter, compare);
	begin = piter + 1;
    }
}

}; // core::sort

using std::cout, std::endl;
using namespace core;

int main(int argc, const char *argv[]) {
    // size_t nworkers = argc < 2 ? 2 : atoi(argv[1]);
    size_t nrecords = argc < 2 ? 10'000'000 : atoi(argv[1]);
    
    std::uniform_int_distribution<uint64_t> d;
    std::mt19937_64 rng;
    std::vector<uint64_t> data(nrecords);
    std::generate(data.begin(), data.end(), [&]() { return d(rng); });

    timer::Timer timer;
    timer.start();
    sort::qsort(data.begin(), data.end(), std::less{});
    timer.stop();
    cout << (1e-9 * timer.elapsed().count()) << endl;

    for (auto i = 1; i < data.size(); ++i)
	if (data[i-1] > data[i])
	    cout << "not sorted : " << i << " " << data[i-1] << " " << data[i]  << endl;
    
    return 0;
}
