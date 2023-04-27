// Copyright (C) 2022, 2023 by Mark Melton
//
#include <random>
#include <span>
#include <stdlib.h>
#include "core/util/tool.h"
#include "core/chrono/stopwatch.h"
#include "core/sort/bitonic.h"
#include "core/sort/is_sorted.h"
#include "core/sort/fixed_sort.h"
#include "core/sort/merge_sort.h"
#include "core/sort/quick_block_sort.h"
#include "core/sort/quick_sort.h"
#include "core/sort/radix_sort_index.h"
#include "core/sort/radix_mem_sort_index.h"
#include "core/sort/std_sort_index.h"
#include "core/sort/std_sort_pointer.h"

#if defined(__APPLE__) && defined(__MACH__)
#define MACOSX 1
#endif

namespace core::sort {

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
	(cout, "std-sort-pointer",
	 [&]() { return std_sort_pointer(frame, sort_keys); },
	 [&](const auto& ptrs) { return is_sorted(ptrs, sort_keys); });
    
    measure_sort_indirect<Units>
	(cout, "std-sort-index",
	 [&]() { return std_sort_index(frame, sort_keys); },
	 [&](auto index) { return is_sorted(index, frame, sort_keys); });

    measure_sort_indirect<Units>
	(cout, "radix-index",
	 [&]() { return radix_index(frame, sort_keys); },
	 [&](auto index) { return is_sorted(index, frame, sort_keys); });

    measure_sort_indirect<Units>
	(cout, "radix-mem-index",
	 [&]() { return radix_mem_index(frame, sort_keys); },
	 [&](auto index) { return is_sorted(index, frame, sort_keys); });

    auto frame0 = frame.clone();
    measure_sort<Units>
	(cout, "merge-bottom-up",
	 [&]() { merge_bottom_up(frame0, sort_keys); },
	 [&]() { return is_sorted(frame0, sort_keys); });

    auto frame1 = frame.clone();
    measure_sort<Units>
	(cout, "quick-sort",
	 [&]() { quick_sort(frame1, sort_keys); },
	 [&]() { return is_sorted(frame1, sort_keys); });
    
    auto frame2 = frame.clone();
    measure_sort<Units>
	(cout, "quick-block-sort",
	 [&]() { quick_block_sort(frame2, sort_keys); },
	 [&]() { return is_sorted(frame2, sort_keys); });

    auto frame3 = frame.clone();
    measure_sort<Units>
	(cout, "qsort_r",
	 [&]() {
	     qsort_r(frame3.begin(), frame3.nrows(), frame3.bytes_per_row(),
#ifdef MACOSX
		     (void*)&sort_keys,
		     [](void *ctx, const void *a, const void *b) -> int {
			 auto *keys = reinterpret_cast<Keys*>(ctx);
			 return compare(a, b, *keys) ? -1 : compare(b, a, *keys);
		     }
#endif
#ifndef MACOSX
		     [](const void *a, const void *b, void *ctx) -> int {
			 auto *keys = reinterpret_cast<Keys*>(ctx);
			 return compare(a, b, *keys) ? -1 : compare(b, a, *keys);
		     }
		     ,(void*)&sort_keys
#endif
		     );
	 },
	 [&]() { return is_sorted(frame3, sort_keys); });

    if (frame.bytes_per_row() == 8
	and sort_keys.size() == 1
	and sort_keys[0].type == DataType::Unsigned64) {
	
	// auto frame1 = frame.clone();
	// timer.mark();
	// stdext::bitsetsort(frame1.row(0), frame1.row(frame1.nrows()),
	// 		   [](const uint64_t& a, const uint64_t& b) {
	//     return a < b;
	// });
	// if (verbose) {
	//     auto millis = timer.elapsed_duration<std::chrono::milliseconds>().count();
	//     cout << fmt::format("bitset-sort: {}ms", millis) << endl;
	// }
	// if (not check_sort(frame1, sort_keys))
	//     throw core::runtime_error("bitset sort failed");
	
	timer.mark();
	auto ptr = reinterpret_cast<uint64_t*>(frame.data());
	std::sort(ptr, ptr + frame.nrows(), [](const uint64_t& a, const uint64_t& b) {
	    return a < b;
	});
	if (verbose) {
	    auto millis = timer.elapsed_duration<std::chrono::milliseconds>().count();
	    cout << fmt::format("vanilla-sort: {}ms", millis) << endl;
	}
	if (not is_sorted(frame, sort_keys))
	    throw core::runtime_error("vanilla sort failed");

    }

    return 0;
}
