// Copyright (C) 2022, 2023 by Mark Melton
//

#include <iostream>
#include <random>
#include <fmt/format.h>
#include "core/timer/timer.h"
#include "core/record/iterator.h"
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

template<class Work>
void measure_record(std::string_view desc, size_t record_size, size_t nrecords, Work&& work) {
    std::uniform_int_distribution<uint64_t> d;
    std::mt19937_64 rng;
    std::vector<uint64_t> data(record_size * nrecords);
    std::generate(data.begin(), data.end(), [&]() { return d(rng); });
    
    timer::Timer timer;
    timer.start();
    work(record::begin(data, record_size), record::end(data, record_size));
    timer.stop();

    cout << fmt::format("{:10d} {:3d} {:.3f} {:s}",
			nrecords, record_size, 1e-9 * timer.elapsed().count(), desc)
	 << endl;

    auto iter = record::begin(data, record_size);
    for (auto i = 1; i < nrecords; ++i) {
	if (iter[i-1][0] > iter[i][0]) {
	    cout << desc << " not sorted : " << i << " "
		 << iter[i-1][0] << " " << iter[i][0]  << endl;
	    throw std::runtime_error("not sorted");
	}
    }
}

int main(int argc, const char *argv[]) {
    size_t nrecords = argc < 2 ? 10'000'000 : atoi(argv[1]);

    measure("std::sort", nrecords, [](auto begin, auto end) {
	std::sort(begin, end);
    });
    for (auto k = 1; k <= 16; k *= 2)
	measure_record("std::sort", k, nrecords, [](auto begin, auto end) {
	    std::sort(begin, end, [](const uint64_t *a, const uint64_t *b) {
		return a[0] < b[0];
	    });
	});
    
    measure("boost::sort::flat_stable_sort", nrecords, [](auto begin, auto end) {
	boost::sort::flat_stable_sort(begin, end);
    });
    
    measure("boost::sort::pdqsort", nrecords, [](auto begin, auto end) {
	boost::sort::pdqsort(begin, end);
    });
    for (auto k = 1; k <= 16; k *= 2)
	measure_record("boost::sort::pdqsort", k, nrecords, [](auto begin, auto end) {
	    boost::sort::pdqsort(begin, end, [](const uint64_t *a, const uint64_t *b) {
		return a[0] < b[0];
	    });
	});
    
    measure("boost::sort::spinsort", nrecords, [](auto begin, auto end) {
	boost::sort::spinsort(begin, end);
    });
    // for (auto k = 1; k <= 16; k *= 2)
    // 	measure_record("boost::sort::spinsort", k, nrecords, [](auto begin, auto end) {
    // 	    boost::sort::spinsort(begin, end, [](const uint64_t *a, const uint64_t *b) {
    // 		return a[0] < b[0];
    // 	    });
    // 	});

    measure("boost::sort::block_indirect_sort", nrecords, [](auto begin, auto end) {
	boost::sort::block_indirect_sort(begin, end);
    });
    // for (auto k = 1; k <= 16; k *= 2)
    // 	measure_record("boost::sort::block_indirect_sort", k, nrecords, [](auto begin, auto end) {
    // 	    boost::sort::block_indirect_sort(begin, end, [](const uint64_t *a, const uint64_t *b) {
    // 		return a[0] < b[0];
    // 	    });
    // 	});
    
    measure("boost::sort::sample_sort", nrecords, [](auto begin, auto end) {
	boost::sort::sample_sort(begin, end);
    });
    
    measure("boost::sort::parallel_stable_sort", nrecords, [](auto begin, auto end) {
	boost::sort::parallel_stable_sort(begin, end);
    });
    
    return 0;
}
