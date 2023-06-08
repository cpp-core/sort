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
       inline constexpr size_t PseudoMedianThreshold = 128;
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
auto partition0(Iter begin, Iter end, Compare compare) {
    using std::swap;
    using T = typename Iter::value_type;
    
    T pivot = std::move(*begin);
    Iter liter = begin;
    Iter riter = end;

    alignas(CacheLineSize) std::array<unsigned char, BlockSize> loffsets;
    alignas(CacheLineSize) std::array<unsigned char, BlockSize> roffsets;
    size_t lcount{}, rcount{}, lslot{}, rslot{};
    Iter lbase = liter + 1, rbase = riter - 1;
    while (riter - liter > 1) {
	size_t loff = liter + 1 - lbase;
	size_t roff = riter + 1 - rbase;
	assert(loff <= 256);
	assert(roff <= 256);
	
	size_t nspace = riter - liter - 1;
	size_t nl = std::min(256 - loff, BlockSize - lcount);
	size_t nr = std::min(256 - roff, BlockSize - rcount);

	if (nl + nr > nspace) {
	    auto& a = (nl < nr ? nl : nr);
	    auto& b = (nl < nr ? nr : nl);
	    a = std::min(a, nspace / 2);
	    b = nspace - a;
	}
	assert(nl + nr <= nspace);

	auto ldx0 = lslot;
	for (auto i = 0; i < nl; ++i) {
	    loffsets[lslot % BlockSize] = loff++;
	    lslot += not compare(*++liter, pivot);
	}
	lcount += lslot - ldx0;

	auto rdx0 = rslot;
	for (auto i = 0; i < nr; ++i) {
	    roffsets[rslot % BlockSize] = roff++;
	    rslot += compare(*--riter, pivot);
	}
	rcount += rslot - rdx0;

	auto n = std::min(lcount, rcount);
	auto l0 = (lslot + BlockSize - lcount) % BlockSize;
	auto r0 = (rslot + BlockSize - rcount) % BlockSize;
	for (auto i = 0; i < n; ++i) {
	    auto lindex = (l0 + i) % BlockSize;
	    auto rindex = (r0 + i) % BlockSize;
	    std::iter_swap(lbase + loffsets[lindex], rbase - roffsets[rindex]);
	}

	lcount -= n;
	rcount -= n;

	if (lcount == 0) {
	    lslot = 0;
	    lbase = liter + 1;
	    loff = 0;
	}
	if (rcount == 0) {
	    rslot = 0;
	    rbase = riter - 1;
	    roff = 0;
	}
    }

    if (lslot) {
	while (lcount--)
	    std::iter_swap(lbase + loffsets[(lslot + lcount) % BlockSize], --riter);
	liter = riter - 1;
    }

    if (rslot) {
	while (rcount--)
	    std::iter_swap(rbase - roffsets[(rslot + rcount) % BlockSize], ++liter);
	riter = liter + 1;
    }

    Iter piter = liter;
    *begin = std::move(*piter);
    *piter = std::move(pivot);

    return piter;
}

template<class Iter, class Compare>
auto partition(Iter begin, Iter end, Compare compare) {
    using std::swap;
    using T = typename Iter::value_type;
    
    T pivot = std::move(*begin);
    Iter liter = begin;
    Iter riter = end;

    alignas(CacheLineSize) std::array<unsigned char, BlockSize> loffsets;
    alignas(CacheLineSize) std::array<unsigned char, BlockSize> roffsets;
    unsigned char *lptroff = &loffsets[0];
    unsigned char *rptroff = &roffsets[0];
    size_t ldx{}, rdx{};
    Iter lbase = liter + 1, rbase = riter - 1;
    while (riter - liter > 2 * BlockSize) {

	if (ldx == 0) {
	    lbase = liter + 1;
	    for (auto i = 0; i < BlockSize;) {
		loffsets[ldx] = i++; ldx += not compare(*++liter, pivot);
		loffsets[ldx] = i++; ldx += not compare(*++liter, pivot);
		loffsets[ldx] = i++; ldx += not compare(*++liter, pivot);
		loffsets[ldx] = i++; ldx += not compare(*++liter, pivot);
		loffsets[ldx] = i++; ldx += not compare(*++liter, pivot);
		loffsets[ldx] = i++; ldx += not compare(*++liter, pivot);
		loffsets[ldx] = i++; ldx += not compare(*++liter, pivot);
		loffsets[ldx] = i++; ldx += not compare(*++liter, pivot);
	    }
	}
	
	if (rdx == 0) {
	    rbase = riter - 1;
	    for (auto i = 0; i < BlockSize;) {
		roffsets[rdx] = i++; rdx += compare(*--riter, pivot);
		roffsets[rdx] = i++; rdx += compare(*--riter, pivot);
		roffsets[rdx] = i++; rdx += compare(*--riter, pivot);
		roffsets[rdx] = i++; rdx += compare(*--riter, pivot);
		roffsets[rdx] = i++; rdx += compare(*--riter, pivot);
		roffsets[rdx] = i++; rdx += compare(*--riter, pivot);
		roffsets[rdx] = i++; rdx += compare(*--riter, pivot);
		roffsets[rdx] = i++; rdx += compare(*--riter, pivot);
	    }
	}

	auto n = std::min(ldx, rdx);
	for (auto i = 0; i < n; ++i)
	    swap(lbase[lptroff[i]], rbase[-rptroff[i]]);

	ldx -= n;
	rdx -= n;

	lptroff = (ldx == 0 ? &loffsets[0] : lptroff + n);
	rptroff = (rdx == 0 ? &roffsets[0] : rptroff + n);
    }
    
    while (riter - liter > 1) {
	size_t nspace = riter - liter - 1;
	size_t nl = ldx == 0 ? (rdx == 0 ? nspace / 2 : nspace) : 0;
	size_t nr = rdx == 0 ? (nspace - nl) : 0;

	nl = std::min(nl, BlockSize);
	nr = std::min(nr, BlockSize);
	
	lbase = nl ? liter + 1 : lbase;
	rbase = nr ? riter - 1 : rbase;

	for (auto i = 0; i < nl; ++i) {
	    loffsets[ldx] = i;
	    ldx += not compare(*++liter, pivot);
	}

	for (auto i = 0; i < nr; ++i) {
	    roffsets[rdx] = i;
	    rdx += compare(*--riter, pivot);
	}

	auto n = std::min(ldx, rdx);
	for (auto i = 0; i < n; ++i)
	    swap(lbase[lptroff[i]], rbase[-rptroff[i]]);

	ldx -= n;
	rdx -= n;

	lptroff = (ldx == 0 ? &loffsets[0] : lptroff + n);
	rptroff = (rdx == 0 ? &roffsets[0] : rptroff + n);
    }

    if (ldx) {
	while (ldx--)
	    std::iter_swap(lbase + lptroff[ldx], --riter);
	liter = riter - 1;
    }

    if (rdx) {
	while (rdx--)
	    std::iter_swap(rbase - rptroff[rdx], ++liter);
	riter = liter + 1;
    }

    Iter piter = liter;
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
	auto piter = core::sort::partition0(begin, end, compare);

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
