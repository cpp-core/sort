// Copyright (C) 2022, 2023 by Mark Melton
//

#include <iostream>
#include <random>
#include <fmt/format.h>
#include "core/timer/timer.h"
#include "core/sort/qsort.h"
#include "boost/sort/sort.hpp"
#include "core/argparse/argp.h"
#include "core/lexical_cast/lexical_cast.h"

using std::cout, std::endl;
using core::argp::ArgParse, core::argp::argFlag, core::argp::argValue;
using namespace core;

struct NumberRange {
    NumberRange(size_t n)
	: start(n)
	, stop(n)
	, step(1) {
    }
    
    NumberRange(size_t arg_start, size_t arg_stop, size_t arg_step = 1)
	: start(arg_start)
	, stop(arg_stop)
	, step(arg_step) {
    }
    
    size_t start, stop, step;
};

template<class T>
auto split(std::string_view str, char delim) {
    std::vector<T> strs;
    for (size_t begin = 0; begin < str.size();) {
	auto tail = str.substr(begin);
	auto end = tail.find(delim);
	strs.emplace_back(core::lexical_cast<T>(tail.substr(0, end)));
	begin = end == std::string_view::npos ? str.size() : begin + end + 1;
    }
    return strs;
}

namespace core::lexical_cast_detail {
template<>
struct lexical_cast_impl<NumberRange> {
    static NumberRange parse(std::string_view input) {
	auto values = split<uint64_t>(input, ':');
	auto start = values[0];
	auto stop = values.size() > 1 ? values[1] : start;
	auto step = values.size() > 2 ? values[2] : 1;
	return NumberRange(start, stop, step);
    }
};
}; // core::lexical_cast_detail

template<class Work>
void measure(std::string_view desc, size_t nrecords, Work&& work) {
    std::uniform_int_distribution<uint64_t> d;
    std::mt19937_64 rng;
    std::vector<uint64_t> data(nrecords);
    std::generate(data.begin(), data.end(), [&]() { return d(rng); });
    
    timer::Timer timer;
    timer.start();
    work(data.begin(), data.end());
    timer.stop();

    cout << fmt::format("{:10d} {:3d} {:.3f} {:s}",
			nrecords, 1, 1e-9 * timer.elapsed().count(), desc)
	 << endl;

    for (auto i = 1; i < data.size(); ++i) {
	if (data[i-1] > data[i]) {
	    cout << desc << " not sorted : " << i << " " << data[i-1] << " " << data[i]  << endl;
	    throw std::runtime_error("not sorted");
	}
    }
}

int main(int argc, const char *argv[]) {
    ArgParse opts
	(
	 argValue<'n'>("counts", NumberRange{1000}, "Range of record count"),
	 argFlag<'v'>("verbose", "Verbose diagnostics")
	 );
    opts.parse(argc, argv);
    auto counts = opts.get<'n'>();
    // auto verbose = opts.get<'v'>();

    for (auto nrecords = counts.start; nrecords <= counts.stop; nrecords += counts.step) {

	measure("std::sort", nrecords, [](auto begin, auto end) {
	    std::sort(begin, end);
	});
	
	measure("core::sort::qsort", nrecords, [](auto begin, auto end) {
	    sort::qsort(begin, end);
	});
	
	measure("boost::sort::flat_stable_sort", nrecords, [](auto begin, auto end) {
	    boost::sort::flat_stable_sort(begin, end);
	});
	
	measure("boost::sort::pdqsort", nrecords, [](auto begin, auto end) {
	    boost::sort::pdqsort(begin, end);
	});
	
	measure("boost::sort::spinsort", nrecords, [](auto begin, auto end) {
	    boost::sort::spinsort(begin, end);
	});
	
	measure("boost::sort::spreadsort", nrecords, [](auto begin, auto end) {
	    boost::sort::spinsort(begin, end);
	});
    }

    return 0;
}
