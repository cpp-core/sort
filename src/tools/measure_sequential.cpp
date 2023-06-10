// Copyright (C) 2022, 2023 by Mark Melton
//

#include <iostream>
#include <random>
#include <fmt/format.h>
#include "core/timer/timer.h"
#include "core/sort/qsort.h"
#include "boost/sort/sort.hpp"

using std::cout, std::endl;
using namespace core;

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
    size_t nrecords = argc < 2 ? 10'000'000 : atoi(argv[1]);

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

    return 0;
}
