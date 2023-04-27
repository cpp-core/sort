// Copyright (C) 2022, 2023 by Mark Melton
//

#pragma once
#include <algorithm>
#include <fmt/format.h>
#include "frame.h"
#include "key.h"
#include "fixed_sort.h"
#include "insertion_sort.h"

namespace core::sort {

int quick_block_sort_partition(Frame& frame, const Keys& keys, int ldx, int rdx) {
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

void quick_block_sort(Frame& frame, const Keys& keys, int ldx, int rdx) {
    conditional_swap cswap(conditional_swap([&](auto x, auto y) {
     	return compare(x, y, keys);
    }));

    const auto bpr = frame.bytes_per_row();
    constexpr auto InsertionThreshold = 16;
    constexpr auto FixedThreshold = 8;
    if (ldx >= 0 and rdx >= 0 and ldx < rdx) {
	auto pdx = quick_block_sort_partition(frame, keys, ldx, rdx);

	auto lsize = 1 + pdx - ldx;
	if (lsize < FixedThreshold) fixed_sortUpTo8(frame.row(ldx), lsize, bpr, cswap);
	else if (lsize < InsertionThreshold) insertion_sort(frame, keys, ldx, pdx);
	else quick_block_sort(frame, keys, ldx, pdx);

	auto rsize = rdx - pdx;
	if (rsize < FixedThreshold) fixed_sortUpTo8(frame.row(pdx + 1), rdx - pdx, bpr, cswap);
	else if (rsize < InsertionThreshold) insertion_sort(frame, keys, pdx + 1, rdx);
	else quick_block_sort(frame, keys, pdx + 1, rdx);
    }
}
   
void quick_block_sort(Frame& frame, const Keys& keys) {
    quick_block_sort(frame, keys, 0, frame.nrows() - 1);
}

}; // core::sort
