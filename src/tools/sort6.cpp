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

template<class T, size_t Size, size_t Align>
class array_ring {
public:
    auto size() const {
	return head_ - tail_;
    }

    auto& next() {
	return data_[head_ % Size];
    }

    auto& front() {
	return data_[tail_ % Size];
    }

    const auto& front() const {
	return data_[tail_ % Size];
    }

    auto& back() {
	return data_[(head_ + BlockSize - 1) % Size];
    }

    const auto& back() const {
	return data_[(head_ + BlockSize - 1) % Size];
    }

    auto& head_index() {
	return head_;
    }

    const auto& head_index() const {
	return head_;
    }

    auto& tail_index() {
	return tail_;
    }
    
    const auto& tail_index() const {
	return tail_;
    }

    T pop_front() {
	return data_[tail_++ % Size];
    }
    
    void push_back(const T& value) {
	data_[head_++ % Size] = value;
    }

    T pop_back() {
	return data_[--head_ % Size];
    }

private:
    alignas(Align) std::array<T, Size> data_;
    size_t head_{}, tail_{};
};

template<class Iter, class Compare>
void output_partition(std::ostream& os, Iter base, Iter begin, Iter end, auto pivot,
		      Compare compare) {
    for (auto iter = begin; iter < end; ++iter) {
	os << (iter - base) << " "
	   << (compare(*iter, pivot) ? " * " : "  ") << " "
	   << *iter << " "
	   << std::endl;
    }
}

template<class Iter, class Compare>
bool paritition_ordered(Iter begin, Iter end, auto pivot, Compare compare, bool reverse = false) {
    for (auto iter = begin; iter < end; ++iter)
	if (not (compare(*iter, pivot) ^ reverse))
	    return false;
    return true;
}

template<class Iter, class Compare>
auto partition0(Iter begin, Iter end, Compare compare) {
    using std::swap;
    using T = typename Iter::value_type;
    
    T pivot = std::move(*begin);
    Iter liter = begin;
    Iter riter = end;

    array_ring<unsigned char, BlockSize, CacheLineSize> lring, rring;
    Iter lbase, rbase;
    while (riter - liter > 1) {
	if (lring.size() == 0)
	    lbase = liter + 1;

	if (rring.size() == 0)
	    rbase = riter - 1;

	size_t loff = liter - lbase + 1;
	size_t roff = rbase - riter + 1;
	
	size_t nspace = riter - liter - 1;
	size_t nl = std::min(256 - loff, BlockSize - lring.size());
	size_t nr = std::min(256 - roff, BlockSize - rring.size());

	if (nl + nr > nspace) {
	    auto& a = (nl < nr ? nl : nr);
	    auto& b = (nl < nr ? nr : nl);
	    a = std::min(a, nspace / 2);
	    b = nspace - a;
	}

	constexpr auto Unroll = 4;
	
	if (nl > 0) {
	    while (nl >= Unroll) {
		lring.next() = loff++; lring.head_index() += not compare(*++liter, pivot);
		lring.next() = loff++; lring.head_index() += not compare(*++liter, pivot);
		lring.next() = loff++; lring.head_index() += not compare(*++liter, pivot);
		lring.next() = loff++; lring.head_index() += not compare(*++liter, pivot);
		nl -= Unroll;
	    }
	    while (nl-- > 0) {
		lring.next() = loff++;
		lring.head_index() += not compare(*++liter, pivot);
	    }
	}

	if (nr > 0) {
	    while (nr >= Unroll) {
		rring.next() = roff++; rring.head_index() += compare(*--riter, pivot);
		rring.next() = roff++; rring.head_index() += compare(*--riter, pivot);
		rring.next() = roff++; rring.head_index() += compare(*--riter, pivot);
		rring.next() = roff++; rring.head_index() += compare(*--riter, pivot);
		nr -= Unroll;
	    }
	    while (nr-- > 0) {
		rring.next() = roff++;
		rring.head_index() += compare(*--riter, pivot);
	    }
	}

	auto n = std::min(lring.size(), rring.size());
	for (auto i = 0; i < n; ++i)
	    std::iter_swap(lbase + lring.pop_front(), rbase - rring.pop_front());
    }

    if (lring.size() > 0) {
	while (lring.size())
	    std::iter_swap(lbase + lring.pop_back(), --riter);
	liter = riter - 1;
    }
    
    if (rring.size() > 0) {
	while (rring.size())
	    std::iter_swap(rbase - rring.pop_back(), ++liter);
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
	size_t nl = ldx == 0 ? std::min(rdx == 0 ? nspace / 2 : nspace, BlockSize) : 0;
	size_t nr = rdx == 0 ? std::min(nspace - nl, BlockSize) : 0;

	constexpr auto Unroll = 4;

	if (nl > 0) {
	    lbase = liter + 1;
	    size_t loff{};
	    while (nl >= Unroll) {
		loffsets[ldx] = loff++; ldx += not compare(*++liter, pivot);
		loffsets[ldx] = loff++; ldx += not compare(*++liter, pivot);
		loffsets[ldx] = loff++; ldx += not compare(*++liter, pivot);
		loffsets[ldx] = loff++; ldx += not compare(*++liter, pivot);
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
		roffsets[rdx] = roff++; rdx += compare(*--riter, pivot);
		roffsets[rdx] = roff++; rdx += compare(*--riter, pivot);
		roffsets[rdx] = roff++; rdx += compare(*--riter, pivot);
		roffsets[rdx] = roff++; rdx += compare(*--riter, pivot);
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
