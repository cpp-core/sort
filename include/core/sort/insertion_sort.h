// Copyright (C) 2022, 2023 by Mark Melton
//

#pragma once
#include <algorithm>
#include <fmt/format.h>
#include "frame.h"
#include "key.h"

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

}; // core::sort
