// Copyright (C) 2022, 2023 by Mark Melton
//

#pragma once
#include "frame.h"
#include "key.h"

namespace core::sort {

template<class T, class Compare>
bool is_sorted(const T *data, size_t n, size_t l, Compare compare) {
    const auto *ptr = reinterpret_cast<const uint8_t*>(data);
    const auto *end = ptr + n * l;
    const auto *last_ptr = ptr;
    ptr += l;
    while (ptr < end) {
	if (compare(ptr, last_ptr))
	    return false;
	last_ptr = ptr;
	ptr += l;
    }
    return true;
}

bool is_sorted(const Frame& frame, const Keys& sort_keys) {
    return is_sorted(frame.row(0), frame.nrows(), frame.bytes_per_row(),
		     [&](const auto *a, const auto *b) {
			 return compare(a, b, sort_keys);
		     });
}

bool is_sorted(const std::vector<int>& index, const Frame& frame, const Keys& sort_keys) {
    return is_sorted(frame.order_by(index), sort_keys);
}

bool is_sorted(const std::vector<const ElementType*>& ptrs, const Keys& sort_keys) {
    for (auto i = 1; i < ptrs.size(); ++i)
	if (compare(ptrs[i], ptrs[i-1], sort_keys))
	    return false;
    return true;
}

}; // core::sort
