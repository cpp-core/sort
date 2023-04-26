// Copyright (C) 2022, 2023 by Mark Melton
//

#pragma once
#include <algorithm>
#include "frame.h"
#include "key.h"

namespace core::sort {

auto radix_index(Frame& frame, const Keys& sort_keys) {
    using RadixIndex = int;
    constexpr auto RadixSize = 257;

    using SortIndex = std::vector<RadixIndex>;
    
    Keys keys = sort_keys;
    std::reverse(keys.begin(), keys.end());
    
    const auto key_length = total_key_length(keys);
    RadixIndex buckets[key_length][RadixSize];
    memset(buckets, 0, key_length * RadixSize * sizeof(RadixIndex));

    for (auto i = 0; i < frame.nrows(); ++i) {
	auto row = frame.row(i);
	auto bdx = 0;
	for (const auto& key : keys) {
	    auto field = row + key.offset;
	    switch (key.type) {
		using enum DataType;
	    case Unsigned8:
		++buckets[bdx++][1 + field[0]];
		break;
	    case Unsigned16:
		for (auto i = 0; i < 2; ++i)
		    ++buckets[bdx++][1 + field[i]];
		break;
	    case Unsigned32:
		for (auto i = 0; i < 4; ++i)
		    ++buckets[bdx++][1 + field[i]];
		break;
	    case Unsigned64:
		for (auto i = 0; i < 8; ++i)
		    ++buckets[bdx++][1 + field[i]];
		break;
	    case Signed64:
		for (auto i = 0; i < 8; ++i)
		    ++buckets[bdx++][1 + field[i]];
		break;
	    }
	}
    }

    for (auto i = 0; i < key_length; ++i) {
	auto *counts = buckets[i];
	for (auto j = 1; j < 257; ++j)
	    counts[j] += counts[j - 1];
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
	    for (auto i = 0; i < key.length(); ++i) {
		auto *counts = buckets[bdx++];
		auto offset = key.offset + i;
		for (auto j = 0; j < frame.nrows(); ++j) {
		    auto value = frame[index[j], offset];
		    auto& loc = counts[value];
		    new_index[loc] = index[j];
		    ++loc;
		}
		std::swap(index, new_index);
	    }
	    break;
	case Signed64:
	    for (auto i = 0; i < key.length(); ++i) {
		auto *counts = buckets[bdx++];
		auto offset = key.offset + i;
		for (auto j = 0; j < frame.nrows(); ++j) {
		    auto value = frame[index[j], offset];
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
