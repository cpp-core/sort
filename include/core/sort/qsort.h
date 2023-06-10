// Copyright (C) 2022, 2023 by Mark Melton
//

#pragma once
#include <algorithm>
#include <array>

namespace core::sort {

inline
namespace qsort_detail {

#define REPEAT4(code) code; code; code; code
#define REPEAT8(code) REPEAT4(code); REPEAT4(code)

inline constexpr size_t InsertionSortThreshold = 32;
inline constexpr size_t PseudoMedianThreshold = 128;
inline constexpr size_t CacheLineSize = 64;
inline constexpr size_t BlockSize = 128;

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
bool paritition_ordered(Iter begin, Iter end, auto pivot, Compare compare, bool reverse = false) {
    for (auto iter = begin; iter < end; ++iter)
	if (not (compare(*iter, pivot) ^ reverse))
	    return false;
    return true;
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
		REPEAT8(loffsets[ldx] = i++; ldx += not compare(*++liter, pivot));
	    }
	}
	
	if (rdx == 0) {
	    rbase = riter - 1;
	    for (auto i = 0; i < BlockSize;) {
		REPEAT8(roffsets[rdx] = i++; rdx += compare(*--riter, pivot));
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
	constexpr auto Unroll = 4;
	size_t nspace = riter - liter - 1;
	size_t nl = ldx == 0 ? std::min(rdx == 0 ? nspace / 2 : nspace, BlockSize) : 0;
	size_t nr = rdx == 0 ? std::min(nspace - nl, BlockSize) : 0;


	if (nl > 0) {
	    lbase = liter + 1;
	    size_t loff{};
	    while (nl >= Unroll) {
		REPEAT4(loffsets[ldx] = loff++; ldx += not compare(*++liter, pivot));
		nl -= Unroll;
	    }
	    while (nl-- > 0) {
		loffsets[ldx] = loff++;
		ldx += not compare(*++liter, pivot);
	    }
	}

	if (nr > 0) {
	    rbase = riter - 1;
	    size_t roff{};
	    while (nr >= Unroll) {
		REPEAT4(roffsets[rdx] = roff++; rdx += compare(*--riter, pivot));
		nr -= Unroll;
	    }
	    while (nr-- > 0) {
		roffsets[rdx] = roff++;
		rdx += compare(*--riter, pivot);
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

#undef REPEAT4
#undef REPEAT8

}; // qsort::detail

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

template<class Iter>
void qsort(Iter begin, Iter end) {
    qsort(begin, end, std::less{});
}

}; // core::sort

