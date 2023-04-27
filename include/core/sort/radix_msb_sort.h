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

    Frame buffer = frame.empty_clone();
    std::vector<RadixIndex> raw_key(frame.nrows());
    for (const auto& key : sort_keys) {
	for (auto kdx = 0; kdx < key.length(); ++kdx) {
	    RadixIndex buckets[RadixSize];
	    memset(buckets, 0, RadixSize * sizeof(RadixIndex));

	    for (auto i = 0; i < frame.nrows(); ++i) {
		switch (key.type) {
		    using enum DataType;
		case Unsigned8:
		case Unsigned16:
		case Unsigned32:
		case Unsigned64:
		    {
			auto value = frame[i, key.offset + key.length() - kdx - 1];
			++buckets[1 + value];
			raw_key[i] = value;
			break;
		    }
		case Signed64:
		    {
			auto value = frame[i, key.offset + key.length() - kdx - 1];
			++buckets[1 + value];
			raw_key[i] = value;
			break;
		    }
		}
	    }

	    for (auto i = 1; i < RadixSize; ++i)
		buckets[i] += buckets[i - 1];

	    for (auto i = 0; i < frame.nrows(); ++i) {
		auto value = raw_key[i];
		auto& loc = buckets[value];
		std::copy(frame.row(i), frame.row(i+1), buffer.row(loc));
		++loc;
	    }

	    std::swap(buffer, frame);
	}
    }
}

}; // core::sort
