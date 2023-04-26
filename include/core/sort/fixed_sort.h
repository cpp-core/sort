// Copyright (C) 2022, 2023 by Mark Melton
//

#pragma once
#include <algorithm>

namespace core::sort {

template<class Compare>
class conditional_swap {
public:
    conditional_swap(Compare&& cmp)
	: _compare(std::forward<Compare>(cmp)) {
    }
    
    inline void operator()(auto a, auto b, size_t l) const {
	if (not _compare(a, b))
	    std::swap_ranges(a, a + l, b);
    }
private:
    Compare _compare;
};

template<class T, class ConditionalSwap>
void fixed_sort2(T *a, size_t l, ConditionalSwap cswap) {
    cswap(a + 0 * l, a + 1 * l, l);
}

template<class T, class ConditionalSwap>
void fixed_sort3(T *a, size_t l, ConditionalSwap cswap) {
    cswap(a + 1 * l, a + 2 * l, l);
    cswap(a + 0 * l, a + 2 * l, l);
    cswap(a + 0 * l, a + 1 * l, l);
}

template<class T, class ConditionalSwap>
void fixed_sort4(T *a, size_t l, ConditionalSwap cswap) {
    cswap(a + 0 * l, a + 1 * l, l);
    cswap(a + 2 * l, a + 3 * l, l);
    cswap(a + 0 * l, a + 2 * l, l);
    cswap(a + 1 * l, a + 3 * l, l);
    cswap(a + 1 * l, a + 2 * l, l);
}

template<class T, class ConditionalSwap>
void fixed_sort5(T *a, size_t l, ConditionalSwap cswap) {
    cswap(a + 0 * l, a + 1 * l, l);
    cswap(a + 3 * l, a + 4 * l, l);
    cswap(a + 2 * l, a + 4 * l, l);
    cswap(a + 2 * l, a + 3 * l, l);
    cswap(a + 0 * l, a + 3 * l, l);
    cswap(a + 1 * l, a + 4 * l, l);
    cswap(a + 0 * l, a + 2 * l, l);
    cswap(a + 1 * l, a + 3 * l, l);
    cswap(a + 1 * l, a + 2 * l, l);
}

template<class T, class ConditionalSwap>
void fixed_sort6(T *a, size_t l, ConditionalSwap cswap) {
  cswap(a + 1 * l, a + 2 * l, l);
  cswap(a + 4 * l, a + 5 * l, l);
  cswap(a + 0 * l, a + 2 * l, l);
  cswap(a + 3 * l, a + 5 * l, l);
  cswap(a + 0 * l, a + 1 * l, l);
  cswap(a + 3 * l, a + 4 * l, l);
  cswap(a + 0 * l, a + 3 * l, l);
  cswap(a + 1 * l, a + 4 * l, l);
  cswap(a + 2 * l, a + 5 * l, l);
  cswap(a + 2 * l, a + 4 * l, l);
  cswap(a + 1 * l, a + 3 * l, l);
  cswap(a + 2 * l, a + 3 * l, l);
}
template<class T, class ConditionalSwap>
void fixed_sort7(T *a, size_t l, ConditionalSwap cswap) {
  cswap(a + 1 * l, a + 2 * l, l);
  cswap(a + 3 * l, a + 4 * l, l);
  cswap(a + 5 * l, a + 6 * l, l);
  cswap(a + 0 * l, a + 2 * l, l);
  cswap(a + 3 * l, a + 5 * l, l);
  cswap(a + 4 * l, a + 6 * l, l);
  cswap(a + 0 * l, a + 1 * l, l);
  cswap(a + 4 * l, a + 5 * l, l);
  cswap(a + 0 * l, a + 4 * l, l);
  cswap(a + 1 * l, a + 5 * l, l);
  cswap(a + 2 * l, a + 6 * l, l);
  cswap(a + 0 * l, a + 3 * l, l);
  cswap(a + 2 * l, a + 5 * l, l);
  cswap(a + 1 * l, a + 3 * l, l);
  cswap(a + 2 * l, a + 4 * l, l);
  cswap(a + 2 * l, a + 3 * l, l);
}

template<class T, class ConditionalSwap>
void fixed_sort8(T *a, size_t l, ConditionalSwap cswap) {
  cswap(a + 0 * l, a + 1 * l, l);
  cswap(a + 2 * l, a + 3 * l, l);
  cswap(a + 4 * l, a + 5 * l, l);
  cswap(a + 6 * l, a + 7 * l, l);
  cswap(a + 0 * l, a + 2 * l, l);
  cswap(a + 1 * l, a + 3 * l, l);
  cswap(a + 4 * l, a + 6 * l, l);
  cswap(a + 5 * l, a + 7 * l, l);
  cswap(a + 1 * l, a + 2 * l, l);
  cswap(a + 5 * l, a + 6 * l, l);
  cswap(a + 0 * l, a + 4 * l, l);
  cswap(a + 1 * l, a + 5 * l, l);
  cswap(a + 2 * l, a + 6 * l, l);
  cswap(a + 3 * l, a + 7 * l, l);
  cswap(a + 1 * l, a + 4 * l, l);
  cswap(a + 3 * l, a + 6 * l, l);
  cswap(a + 2 * l, a + 4 * l, l);
  cswap(a + 3 * l, a + 5 * l, l);
  cswap(a + 3 * l, a + 4 * l, l);
}

template<class T, class ConditionalSwap>
void fixed_sortUpTo8(T *a, size_t n, size_t l, ConditionalSwap cswap) {
    switch (n) {
    case 0:
    case 1:
	return;
    case 2:
	fixed_sort2(a, l, cswap);
	return;
    case 3:
	fixed_sort3(a, l, cswap);
	return;
    case 4:
	fixed_sort4(a, l, cswap);
	return;
    case 5:
	fixed_sort5(a, l, cswap);
	return;
    case 6:
	fixed_sort6(a, l, cswap);
	return;
    case 7:
	fixed_sort7(a, l, cswap);
	return;
    case 8:
	fixed_sort8(a, l, cswap);
	return;
    }
}

}; // core::sort
