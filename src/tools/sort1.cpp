// Copyright (C) 2022, 2023 by Mark Melton
//

#include <random>
#include <span>
#include "core/util/tool.h"
#include "core/chrono/stopwatch.h"
#include "frame.h"
#include "key.h"

using SortIndex = std::vector<int>;

auto pointer_sort(Frame& frame, const Keys& sort_keys) {
    std::vector<const ElementType*> ptrs;
    for (auto ptr : frame)
	ptrs.push_back(ptr);
    
    std::sort(ptrs.begin(), ptrs.end(), [&](const ElementType *a_ptr, const ElementType *b_ptr) {
	return compare(a_ptr, b_ptr, sort_keys);
    });
    
    return ptrs;
}

auto index_sort(Frame& frame, const Keys& sort_keys) {
    SortIndex index(frame.nrows());
    for (auto i = 0; i < frame.nrows(); ++i)
	index[i] = i;

    auto ptr = frame.data();
    std::sort(index.begin(), index.end(), [&](int idx, int jdx) {
	return compare(ptr + idx * frame.bytes_per_row(), ptr + jdx * frame.bytes_per_row(), sort_keys);
    });

    return index;
}

size_t total_key_length(const Keys& keys) {
    size_t n{};
    for (const auto& key : keys)
	n += key.length();
    return n;
}

using RadixIndex = int;
constexpr auto RadixSize = 257;

auto radix_sort(Frame& frame, const Keys& sort_keys) {
    Keys keys = sort_keys;
    std::reverse(keys.begin(), keys.end());
    
    const auto key_length = total_key_length(keys);
    RadixIndex buckets[key_length][RadixSize];
    memset(buckets, 0, key_length * RadixSize * sizeof(RadixIndex));

    for (auto row : frame) {
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

auto radix_sort_dup(Frame& frame, const Keys& sort_keys) {
    Keys keys = sort_keys;
    std::reverse(keys.begin(), keys.end());
    
    const auto key_length = total_key_length(keys);
    RadixIndex buckets[key_length][RadixSize];
    memset(buckets, 0, key_length * RadixSize * sizeof(RadixIndex));

    std::vector<std::vector<uint8_t>> radix_values(key_length);
    for (auto i = 0; i < key_length; ++i)
	radix_values[i].resize(frame.nrows());

    auto idx = 0;
    for (auto row : frame) {
	auto bdx = 0;
	for (const auto& key : keys) {
	    auto field = row + key.offset;
	    switch (key.type) {
		using enum DataType;
	    case Unsigned8:
		radix_values[bdx][idx] = field[0];
		++buckets[bdx++][1 + field[0]];
		break;
	    case Unsigned16:
		for (auto i = 0; i < 2; ++i) {
		    radix_values[bdx][idx] = field[i];
		    ++buckets[bdx++][1 + field[i]];
		}
		break;
	    case Unsigned32:
		for (auto i = 0; i < 4; ++i) {
		    radix_values[bdx][idx] = field[i];
		    ++buckets[bdx++][1 + field[i]];
		}
		break;
	    case Unsigned64:
		for (auto i = 0; i < 8; ++i) {
		    radix_values[bdx][idx] = field[i];
		    ++buckets[bdx++][1 + field[i]];
		}
		break;
	    case Signed64:
		for (auto i = 0; i < 8; ++i) {
		    radix_values[bdx][idx] = field[i];
		    ++buckets[bdx++][1 + field[i]];
		}
		break;
	    }
	}
	++idx;
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
		for (auto j = 1; j < 257; ++j)
		    counts[j] += counts[j - 1];
		auto *values = radix_values[i].data();
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
	    for (auto i = 0; i < key.length(); ++i) {
		auto *counts = buckets[bdx++];
		for (auto j = 1; j < 257; ++j)
		    counts[j] += counts[j - 1];
		auto *values = radix_values[i].data();
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

// void merge_buttom_up_sort(Frame& frame, const Keys& keys) {
//     int n = frame.nrows();
//     std::vector<uint8_t> buffer(n * frame.bytes_per_row());

//     for (auto w = 1; w < n; w *= 2) {
// 	for (auto i = 0; i < n; i += 2 * w) {
// 	    auto max_ldx = i + w, max_rdx = std::min(i + 2 * w, n);
// 	    auto ldx = i, rdx = i + 1, mdx = i;
// 	    while (ldx < max_ldx and rdx < max_rdx) {
// 		if (a[ldx] < a[rdx])
// 		    b[mdx++] = a[ldx++];
// 		else if (a[rdx] < a[ldx])
// 		    b[mdx++] = a[rdx++];
// 	    }
// 	    while (ldx < max_ldx)
// 		b[mdx++] = a[ldx++];
// 	    while (rdx < max_rdx)
// 		b[mdx++] = a[rdx++];
// 	}
// 	std::swap(frame, buffer);
//     }
// }

bool check_sort(const std::vector<const ElementType*>& ptrs, const Keys& sort_keys) {
    for (auto i = 1; i < ptrs.size(); ++i)
	if (not compare(ptrs[i-1], ptrs[i], sort_keys) and compare(ptrs[i], ptrs[i-1], sort_keys))
	    return false;
    return true;
}

bool check_sort(Frame& frame, const SortIndex& index, const Keys& sort_keys) {
    auto ptr = *frame.begin();
    for (auto i = 1; i < index.size(); ++i)
	if (not compare(ptr + index[i-1] * frame.bytes_per_row(),
			ptr + index[i] * frame.bytes_per_row(),
			sort_keys) and
	    compare(ptr + index[i] * frame.bytes_per_row(),
		    ptr + index[i-1] * frame.bytes_per_row(),
		    sort_keys))
	    return false;
    return true;
}

bool check_sort(Frame& frame, const Keys& sort_keys) {
    auto ptr = reinterpret_cast<const uint64_t*>(frame.data());
    for (auto i = 1; i < frame.nrows(); ++i)
	if (not (ptr[i-1] < ptr[i]) and ptr[i] < ptr[i-1])
	    return false;
    return true;
}

int tool_main(int argc, const char *argv[]) {
    ArgParse opts
	(
	 argValue<'n'>("number-rows", (uint64_t)100, "Number of rows"),
	 argValue<'r'>("bytes-per-row", (uint64_t)64, "Bytes per row"),
	 argValues<'*', std::vector, Key>("keys", "Sort keys"),
	 argFlag<'v'>("verbose", "Verbose diagnostics")
	 );
    opts.parse(argc, argv);
    auto number_rows = opts.get<'n'>();
    auto bytes_per_row = opts.get<'r'>();
    auto sort_keys = opts.get<'*'>();
    auto verbose = opts.get<'v'>();

    if (bytes_per_row bitand 0x7)
	throw core::runtime_error("bytes-per-row must be a multple of 8: {}", bytes_per_row);

    if (sort_keys.size() == 0)
	throw core::runtime_error("At least one sort key must be specified");

    if (verbose)
	cout << fmt::format("creating random dataset with {} rows and {} bytes-per-row",
			    number_rows, bytes_per_row)
	     << endl;

    chron::StopWatch timer;
    Frame frame{number_rows, bytes_per_row};
    if (verbose) {
	auto millis = timer.elapsed_duration<std::chrono::milliseconds>().count();
	cout << fmt::format("dataset created: {}ms", millis) << endl;
    }
    
    timer.mark();
    auto ptrs = pointer_sort(frame, sort_keys);
    if (verbose) {
	auto millis = timer.elapsed_duration<std::chrono::milliseconds>().count();
	cout << fmt::format("pointer sort: {}ms", millis) << endl;
    }
    if (not check_sort(ptrs, sort_keys))
	throw core::runtime_error("pointer sort failed");

    timer.mark();
    auto index = index_sort(frame, sort_keys);
    if (verbose) {
	auto millis = timer.elapsed_duration<std::chrono::milliseconds>().count();
	cout << fmt::format("index sort: {}ms", millis) << endl;
    }
    if (not check_sort(frame, index, sort_keys))
	throw core::runtime_error("index sort failed");

    timer.mark();
    auto radix_index = radix_sort(frame, sort_keys);
    if (verbose) {
	auto millis = timer.elapsed_duration<std::chrono::milliseconds>().count();
	cout << fmt::format("radix sort: {}ms", millis) << endl;
    }
    
    if (not check_sort(frame, radix_index, sort_keys))
	throw core::runtime_error("radix sort failed");

    timer.mark();
    auto radix_dup_index = radix_sort_dup(frame, sort_keys);
    if (verbose) {
	auto millis = timer.elapsed_duration<std::chrono::milliseconds>().count();
	cout << fmt::format("radix dup sort: {}ms", millis) << endl;
    }
    
    if (not check_sort(frame, radix_dup_index, sort_keys))
	throw core::runtime_error("radix dup sort failed");

    if (frame.bytes_per_row() == 8 and sort_keys.size() == 1) {
	timer.mark();
	auto ptr = reinterpret_cast<uint64_t*>(frame.data());
	std::sort(ptr, ptr + frame.nrows(), [](const uint64_t& a, const uint64_t& b) {
	    return a < b;
	});
	if (verbose) {
	    auto millis = timer.elapsed_duration<std::chrono::milliseconds>().count();
	    cout << fmt::format("vanilla sort: {}ms", millis) << endl;
	}
	if (not check_sort(frame, sort_keys))
	    throw core::runtime_error("vanilla sort failed");
    }
	
    return 0;
}
