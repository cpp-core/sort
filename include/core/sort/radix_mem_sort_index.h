// Copyright (C) 2022, 2023 by Mark Melton
//

#pragma once
#include <algorithm>
#include "frame.h"
#include "key.h"

namespace core::sort {

auto radix_mem_index(Frame& frame, const Keys& sort_keys) {
    using RadixIndex = int;
    constexpr auto RadixSize = 257;
    using SortIndex = std::vector<int>;
    
    Keys keys = sort_keys;
    std::reverse(keys.begin(), keys.end());
    
    const auto key_length = total_key_length(keys);
    RadixIndex buckets[key_length][RadixSize];
    memset(buckets, 0, key_length * RadixSize * sizeof(RadixIndex));

    std::vector<std::vector<uint8_t>> radix_values(key_length);
    for (auto i = 0; i < key_length; ++i)
	radix_values[i].resize(frame.nrows());

    for (auto i = 0; i < frame.nrows(); ++i) {
	auto row = frame.row(i);
	auto bdx = 0;
	for (const auto& key : keys) {
	    auto field = row + key.offset;
	    switch (key.type) {
		using enum DataType;
	    case Unsigned8:
		radix_values[bdx][i] = field[0];
		++buckets[bdx][1 + field[0]];
		++bdx;
		break;
	    case Unsigned16:
		for (auto j = 0; j < 2; ++j, ++bdx) {
		    radix_values[bdx][i] = field[j];
		    ++buckets[bdx][1 + field[j]];
		}
		break;
	    case Unsigned32:
		for (auto j = 0; j < 4; ++j, ++bdx) {
		    radix_values[bdx][i] = field[j];
		    ++buckets[bdx][1 + field[j]];
		}
		break;
	    case Unsigned64:
		for (auto j = 0; j < 8; ++j, ++bdx) {
		    radix_values[bdx][i] = field[j];
		    ++buckets[bdx][1 + field[j]];
		}
		break;
	    case Signed64:
		for (auto j = 0; j < 8; ++j, ++bdx) {
		    radix_values[bdx][i] = field[j];
		    ++buckets[bdx][1 + field[j]];
		}
		break;
	    }
	}
    }

    SortIndex index(frame.nrows()), new_index(frame.nrows());
    for (auto i = 0; i < frame.nrows(); ++i)
	index[i] = i;

    auto bdx = 0;
    for (const auto& key : keys) {
	switch (key.type) {
	    using enum DataType;
	case Unsigned8:
	case Unsigned16:
	case Unsigned32:
	case Unsigned64:
	    for (auto i = 0; i < key.length(); ++i, ++bdx) {
		auto *counts = buckets[bdx];
		for (auto j = 1; j < RadixSize; ++j)
		    counts[j] += counts[j - 1];
		
		auto *values = radix_values[bdx].data();
		for (auto j = 0; j < frame.nrows(); ++j) {
		    auto value = values[index[j]];
		    auto& loc = counts[value];
		    new_index[loc] = index[j];
		    ++loc;
		}
		std::swap(index, new_index);
	    }
	    break;
	case Signed64:
	    for (auto i = 0; i < key.length(); ++i, ++bdx) {
		auto *counts = buckets[bdx];
		for (auto j = 1; j < RadixSize; ++j)
		    counts[j] += counts[j - 1];
		
		auto *values = radix_values[bdx].data();
		for (auto j = 0; j < frame.nrows(); ++j) {
		    auto value = values[index[j]];
		    auto& loc = counts[value];
		    new_index[loc] = index[j];
		    ++loc;
		}
		std::swap(index, new_index);
	    }
	    break;
	}
    }

    return index;
}

}; // core::sort
