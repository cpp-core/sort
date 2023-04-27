// Copyright (C) 2022, 2023 by Mark Melton
//

#pragma once
#include <algorithm>
#include "frame.h"
#include "key.h"

namespace core::sort {

void radix_msb_sort(Frame& frame, const Keys& sort_keys) {
    using RadixIndex = int;
    constexpr auto RadixSize = 257;

    for (const auto& key : sort_keys) {
	RadixIndex buckets[RadixSize];
	memset(buckets, 0, RadixSize * sizeof(RadixIndex));

	for (auto i = 0; i < frame.nrows(); ++i) {
	    auto value = frame[i, key.offset];
	    switch (key.type) {
		using enum DataType;
	    case Unsigned8:
	    case Unsigned16:
	    case Unsigned32:
	    case Unsigned64:
		++buckets[1 + value];
		break;
	    case Signed64:
		++buckets[1 + value];
		break;
	    }
	}
    }
}

}; // core::sort
