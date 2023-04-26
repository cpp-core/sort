// Copyright (C) 2022, 2023 by Mark Melton
//

#pragma once
#include <algorithm>
#include "frame.h"
#include "key.h"

namespace core::sort {

auto system_index(Frame& frame, const Keys& sort_keys) {
    std::vector<int> index(frame.nrows());
    for (auto i = 0; i < frame.nrows(); ++i)
	index[i] = i;

    auto ptr = frame.data();
    auto bpr = frame.bytes_per_row();
    std::sort(index.begin(), index.end(), [&](int idx, int jdx) {
	return compare(ptr + idx * bpr, ptr + jdx * bpr, sort_keys);
    });

    return index;
}

}; // core::sort
