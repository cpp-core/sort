// Copyright (C) 2022, 2023 by Mark Melton
//

#pragma once
#include <algorithm>
#include "frame.h"
#include "key.h"

namespace core::sort {

template<class Compare>
auto std_sort_index(size_t n, Compare compare) {
    std::vector<int> index(n);
    for (auto i = 0; i < index.size(); ++i)
	index[i] = i;

    std::sort(index.begin(), index.end(), compare);
    return index;
}

auto std_sort_index(Frame& frame, const Keys& sort_keys) {
    auto ptr = reinterpret_cast<uint8_t*>(frame.begin());
    auto l = frame.bytes_per_row();
    return std_sort_index(frame.nrows(), [&](int idx, int jdx) {
	return compare(ptr + idx * l, ptr + jdx * l, sort_keys);
    });
}

}; // core::sort
