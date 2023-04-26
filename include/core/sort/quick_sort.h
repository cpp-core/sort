// Copyright (C) 2022, 2023 by Mark Melton
//

#pragma once
#include <algorithm>
#include <fmt/format.h>
#include "frame.h"
#include "key.h"
#include "fixed_sort.h"

namespace core::sort {

void insertion_sort(Frame& frame, const Keys& keys, int ldx, int rdx) {
    for (auto idx = ldx + 1; idx <= rdx; ++idx) {
	for (auto jdx = idx; jdx > 0 and compare(frame.row(jdx), frame.row(jdx-1), keys); --jdx)
	    frame.swap_rows(jdx, jdx - 1);
    }
}

void insertion_sort(Frame& frame, const Keys& keys) {
    insertion_sort(frame, keys, 0, frame.nrows() - 1);
}

int quick_sort_partition(Frame& frame, const Keys& keys, int ldx, int rdx) {
    auto pdx = ldx + (rdx - ldx) / 2;
    --ldx;
    ++rdx;

    while (true) {
	auto pdx_ptr = frame.row(pdx);
	
	do ++ldx;
	while (compare(frame.row(ldx), pdx_ptr, keys));
	
	do --rdx;
	while (compare(pdx_ptr, frame.row(rdx), keys));

	if (ldx >= rdx)
	    return rdx;

	if (ldx == pdx) pdx = rdx;
	else if (rdx == pdx) pdx = ldx;
	frame.swap_rows(ldx, rdx);
    }
}

void quick_sort(Frame& frame, const Keys& keys, int ldx, int rdx) {
    // conditional_swap cswap(conditional_swap([&](auto x, auto y) {
    // 	return compare(x, y, keys); }));
    constexpr auto Threshold = 8;
    if (ldx >= 0 and rdx >= 0 and ldx < rdx) {
	auto pdx = quick_sort_partition(frame, keys, ldx, rdx);
	
	if (1 + pdx - ldx > Threshold) quick_sort(frame, keys, ldx, pdx);
	else insertion_sort(frame, keys, ldx, pdx);
	// else fixed_sortUpTo8(frame.row(ldx), 1 + pdx - ldx, frame.bytes_per_row(), cswap);
	
	if (rdx - pdx > Threshold) quick_sort(frame, keys, pdx + 1, rdx);
	else insertion_sort(frame, keys, pdx + 1, rdx);
	// else fixed_sortUpTo8(frame.row(pdx + 1), rdx - pdx, frame.bytes_per_row(), cswap);
    }
}
   
void quick_sort(Frame& frame, const Keys& keys) {
    quick_sort(frame, keys, 0, frame.nrows() - 1);
}

}; // core::sort
