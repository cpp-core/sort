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
    os << fmt::format("{:>35.35s}: {:5d} ms", desc, n) << endl;
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
    os << fmt::format("{:>35.35s}: {:5d} ms", desc, n) << endl;
}

}; // core::sort

using namespace core::sort;

bool compare_func(const uint8_t *a, const uint8_t *b, const Keys& keys) {
    return compare(a, b, keys);
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

    bool direct_std_sort = bytes_per_row == 8
	and sort_keys.size() == 1
	and sort_keys[0].type == DataType::Unsigned64;

    using Units = std::chrono::milliseconds;
    
    chron::StopWatch timer;
    Frame frame{number_rows, bytes_per_row};
    if (verbose) {
	auto millis = timer.elapsed_duration<Units>().count();
	cout << fmt::format("dataset created: {}ms", millis) << endl;
    }

    // As a special case, if a record is simply a single uint64_t,
    // directly invoke std::sort passing pointers to the data and a
    // lambda for the direct comparison of the uint64_t values. This
    // represents the best possible performance. In the general case,
    // we will not know the record length or key type at compile time
    // so this method is simply a benchmark for comparison.
    if (direct_std_sort) {
	auto frame1 = frame.clone();
	auto ptr = reinterpret_cast<uint64_t*>(frame1.data());
	measure_sort<std::chrono::milliseconds>
	    (cout, "std::sort-direct",
	     [&]() {
		 std::sort(ptr, ptr + frame.nrows(), [](const uint64_t& a, const uint64_t& b) {
		     return a < b;
		 });
	     },
	     [&]() { return is_sorted(frame1, sort_keys); });
    }

    // Similar to the first case above, but using the generic
    // comparison function instead of assumming a compile-time known
    // key type. This represents a more realistic upper limit on
    // expected performance since in general the key will not be known
    // at compile time. In the general case, we will not know the
    // record length at compile time, so again this mehtod is simply
    // another benchmark fo comparision.
    if (direct_std_sort) {
	auto frame1 = frame.clone();
	auto ptr = reinterpret_cast<uint64_t*>(frame1.data());
	measure_sort<std::chrono::milliseconds>
	    (cout, "std::sort-direct-compare-function",
	     [&]() {
		 std::sort(ptr, ptr + frame.nrows(), [&](const uint64_t& a, const uint64_t& b) {
		     return compare((const uint8_t*)&a, (const uint8_t*)&b, sort_keys);
		 });
	     },
	     [&]() { return is_sorted(frame1, sort_keys); });
    }

    // Use the C library function `qsort_r` and the generic comparison
    // function to directly sort the records. This is a practible method.
    auto frame3 = frame.clone();
    measure_sort<Units>
	(cout, "qsort_r-direct",
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

    // Use std::sort and the generic comparison function to sort a
    // vector of pointers that refers to the actual records.
    measure_sort_indirect<Units>
	(cout, "std::sort-pointer",
	 [&]() { return std_sort_pointer(frame, sort_keys); },
	 [&](const auto& ptrs) { return is_sorted(ptrs, sort_keys); });

    // Use std::sort and the generic comparison function to sort an
    // index vector that refers to the actual records.
    measure_sort_indirect<Units>
	(cout, "std::sort-index",
	 [&]() { return std_sort_index(frame, sort_keys); },
	 [&](auto index) { return is_sorted(index, frame, sort_keys); });

    measure_sort_indirect<Units>
	(cout, "radix-index-sort",
	 [&]() { return radix_index(frame, sort_keys); },
	 [&](auto index) { return is_sorted(index, frame, sort_keys); });

    measure_sort_indirect<Units>
	(cout, "radix-mem-index-sort",
	 [&]() { return radix_mem_index(frame, sort_keys); },
	 [&](auto index) { return is_sorted(index, frame, sort_keys); });

    auto frame0 = frame.clone();
    measure_sort<Units>
	(cout, "merge-bottom-up-sort",
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

    return 0;
}
