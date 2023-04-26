// Copyright (C) 2022, 2023 by Mark Melton
//

#pragma once
#include <algorithm>
#include "frame.h"
#include "key.h"

namespace core::sort {

auto system_pointer(Frame& frame, const Keys& sort_keys) {
    using ElementType = Frame::element_type;
    std::vector<const ElementType*> ptrs;
    for (auto ptr : frame)
	ptrs.push_back(ptr);
    
    std::sort(ptrs.begin(), ptrs.end(), [&](const ElementType *a_ptr, const ElementType *b_ptr) {
	return compare(a_ptr, b_ptr, sort_keys);
    });
    
    return ptrs;
}

}; // core::sort
