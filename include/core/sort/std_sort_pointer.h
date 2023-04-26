// Copyright (C) 2022, 2023 by Mark Melton
//

#pragma once
#include <algorithm>
#include "frame.h"
#include "key.h"

namespace core::sort {

template<class T, class Compare>
auto std_sort_pointer(T *begin, T *end, size_t l, Compare compare) {
    std::vector<const T*> ptrs;
    const auto *ptr = reinterpret_cast<const uint8_t*>(begin);
    while (ptr < end) {
	ptrs.push_back(ptr);
	ptr += l;
    }

    std::sort(ptrs.begin(), ptrs.end(), compare);
    return ptrs;
}

template<class T, class Compare>
auto std_sort_pointer(T *data, size_t n, size_t l, Compare compare) {
    return std_sort_pointers(data, reinterpret_cast<const uint8_t*>(data) + n * l, l, compare);
}

auto std_sort_pointer(Frame& frame, const Keys& sort_keys) {
    return std_sort_pointer(frame.begin(), frame.end(), frame.bytes_per_row(),
			    [&](const auto *a, const auto *b) {
				return compare(a, b, sort_keys);
			    });
}

}; // core::sort
