// Copyright (C) 2022, 2023 by Mark Melton
//

#undef NDEBUG
#include <cassert>

#include <algorithm>
#include <random>
#include <thread>
#include "core/util/tool.h"
#include "core/timer/timer.h"

int tool_main(int argc, const char *argv[]) {
    int nrecords = 100'000'000;
    std::vector<uint64_t> data(nrecords);
    
    std::uniform_int_distribution<uint64_t> d;
    std::mt19937_64 rng;
    std::generate(data.begin(), data.end(), [&]() { return d(rng); });

    size_t nworkers = 10;
    std::vector<std::thread> workers;
    
    core::timer::Timer timer;
    timer.start();

    // std::sort(data.begin(), data.end());
    
    std::vector<uint64_t> sample;
    constexpr size_t blocksize = 64;
    for (auto i = 0; i < data.size(); i += blocksize)
	sample.push_back(data[i]);
    std::sort(sample.begin(), sample.end());

    size_t bucketsize = data.size() / nworkers;
    for (auto i = 0; i < nworkers; ++i) {
	auto sdx = i * bucketsize;
	auto edx = std::min((i + 1) * bucketsize, data.size());
	auto pivot = sample[(i * sample.size()) / nworkers];
	workers.emplace_back([&,sdx,edx,pivot]() {
	    auto ldx = sdx - 1;
	    auto rdx = edx;
	    while (true) {
		do ++ldx;
		while (data[ldx] < pivot);

		do --rdx;
		while (pivot < data[rdx]);

		if (ldx >= rdx)
		    break;
		std::swap(data[ldx], data[rdx]);
	    }
	    std::sort(&data[sdx], &data[rdx]);
	    std::sort(&data[rdx], &data[edx]);
	});
    }

    for (auto& worker : workers)
	worker.join();

    timer.stop();
    cout << (1e-9 * timer.elapsed().count()) << endl;
    
    return 0;
}
