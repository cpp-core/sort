// Copyright (C) 2022, 2023 by Mark Melton
//

#include <random>
#include <span>
#include "core/util/tool.h"
#include "core/chrono/stopwatch.h"
#include "core/sort/bitonic.h"
#include "core/sort/merge.h"
#include "core/sort/quick.h"
#include "core/sort/radix_index.h"
#include "core/sort/radix_mem_index.h"
#include "core/sort/system_pointer.h"
#include "core/sort/system_index.h"

namespace core::sort {

using SortIndex = std::vector<int>;

bool check_sort(const std::vector<const ElementType*>& ptrs, const Keys& sort_keys) {
    for (auto i = 1; i < ptrs.size(); ++i)
	if (not compare(ptrs[i-1], ptrs[i], sort_keys) and compare(ptrs[i], ptrs[i-1], sort_keys))
	    return false;
    return true;
}

bool check_sort(const Frame& frame, const SortIndex& index, const Keys& sort_keys) {
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

bool check_sort(const Frame& frame, const Keys& sort_keys) {
    for (auto i = 1; i < frame.nrows(); ++i) {
	if (not compare(frame.row(i-1), frame.row(i), sort_keys))
	    return false;
	if (compare(frame.row(i), frame.row(i-1), sort_keys))
	    return false;
    }
    return true;
}

template<class Units, class Work, class Check>
size_t measure_sort_indirect(std::string_view desc, Work&& work, Check&& check) {
    chron::StopWatch timer;
    auto result = work();
    if (not check(result))
	throw core::runtime_error("{} failed correctness check", desc);
    return timer.elapsed_duration<Units>().count();
}

template<class Units, class Work, class Check>
void measure_sort_indirect(std::ostream& os, std::string_view desc, Work&& work, Check&& check) {
    auto n = measure_sort_indirect<Units>
	(desc, std::forward<Work>(work), std::forward<Check>(check));
    os << fmt::format("{}: {}ms", desc, n) << endl;
}

template<class Units, class Work, class Check>
size_t measure_sort(std::string_view desc, Work&& work, Check&& check) {
    chron::StopWatch timer;
    work();
    if (not check())
	throw core::runtime_error("{} failed correctness check", desc);
    return timer.elapsed_duration<Units>().count();
}

template<class Units, class Work, class Check>
void measure_sort(std::ostream& os, std::string_view desc, Work&& work, Check&& check) {
    auto n = measure_sort<Units>(desc, std::forward<Work>(work), std::forward<Check>(check));
    os << fmt::format("{}: {}ms", desc, n) << endl;
}

}; // core::sort

using namespace core::sort;

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

    using Units = std::chrono::milliseconds;
    
    chron::StopWatch timer;
    Frame frame{number_rows, bytes_per_row};
    if (verbose) {
	auto millis = timer.elapsed_duration<Units>().count();
	cout << fmt::format("dataset created: {}ms", millis) << endl;
    }

    measure_sort_indirect<Units>
	(cout, "system-pointer",
	 [&]() { return system_pointer(frame, sort_keys); },
	 [&](const auto& ptrs) { return check_sort(ptrs, sort_keys); });
    
    measure_sort_indirect<Units>
	(cout, "system-index",
	 [&]() { return system_index(frame, sort_keys); },
	 [&](auto index) { return check_sort(frame, index, sort_keys); });

    measure_sort_indirect<Units>
	(cout, "radix-index",
	 [&]() { return radix_index(frame, sort_keys); },
	 [&](auto index) { return check_sort(frame, index, sort_keys); });

    measure_sort_indirect<Units>
	(cout, "radix-mem-index",
	 [&]() { return radix_mem_index(frame, sort_keys); },
	 [&](auto index) { return check_sort(frame, index, sort_keys); });

    auto frame0 = frame.clone();
    measure_sort<Units>
	(cout, "merge-bottom-up",
	 [&]() { merge_bottom_up(frame0, sort_keys); },
	 [&]() { return check_sort(frame0, sort_keys); });

    auto frame1 = frame.clone();
    measure_sort<Units>
	(cout, "quick-sort",
	 [&]() { quick_sort(frame1, sort_keys); },
	 [&]() { return check_sort(frame1, sort_keys); });
    
    // auto frame2 = frame.clone();
    // measure_sort<Units>
    // 	(cout, "insertion-sort",
    // 	 [&]() { insertion_sort(frame2, sort_keys); },
    // 	 [&]() { return check_sort(frame2, sort_keys); });
    
    if (frame.bytes_per_row() == 8
	and sort_keys.size() == 1
	and sort_keys[0].type == DataType::Unsigned64) {
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
