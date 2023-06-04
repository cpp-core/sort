// Copyright (C) 2022, 2023 by Mark Melton
//

#undef NDEBUG
#include <cassert>

#include <algorithm>
#include <iostream>
#include <random>
#include <thread>
#include <latch>
#include "core/timer/timer.h"

using std::cout, std::endl;

int main(int argc, const char *argv[]) {
    size_t nworkers = argc < 2 ? 2 : atoi(argv[1]);
    
    int nrecords = 100'000'000;
    std::vector<uint64_t> data(nrecords);
    
    std::uniform_int_distribution<uint64_t> d;
    std::mt19937_64 rng;
    std::generate(data.begin(), data.end(), [&]() { return d(rng); });

    std::vector<std::thread> workers;
    
    core::timer::Timer timer;
    timer.start();

    // std::sort(data.begin(), data.end());
    
    std::vector<uint64_t> sample;
    constexpr size_t blocksize = 64;
    for (auto i = 0; i < data.size(); i += blocksize)
	sample.push_back(data[i]);
    std::sort(sample.begin(), sample.end());

    std::vector<uint64_t> result(data.size());
    size_t bucketsize = data.size() / nworkers;
    auto pivot = sample[sample.size() / 2];
    std::latch wait_here(nworkers);
    for (auto wid = 0; wid < nworkers; ++wid) {
	workers.emplace_back([&,wid,pivot]() {
	    size_t sdx = wid * bucketsize;
	    size_t edx = std::min((wid + 1) * bucketsize, data.size());
	    
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
	    while (rdx < edx and data[rdx] < pivot)
		++rdx;
	    std::sort(&data[sdx], &data[rdx]);
	    std::sort(&data[rdx], &data[edx]);

	    wait_here.arrive_and_wait();

	    auto end_rdx = std::min(edx + bucketsize, data.size());
	    if (wid % 2 == 0) {
		size_t idx{sdx}, ldx{sdx}, rdx{edx};
		while (idx < edx and ldx < edx and rdx < end_rdx) {
		    if (data[ldx] < data[rdx])
			result[idx++] = data[ldx++];
		    else
			result[idx++] = data[rdx++];
		}
	    } else {
		ssize_t idx(edx - 1);
		ssize_t rdx{idx};
		ssize_t ldx(sdx - 1);
		while (idx >= sdx and ldx >= sdx - bucketsize and rdx >= sdx) {
		    if (data[ldx] > data[rdx])
			result[idx--] = data[ldx--];
		    else
			result[idx--] = data[rdx--];
		}
	    }
	});
    }

    for (auto& worker : workers)
	worker.join();
    std::swap(data, result);

    timer.stop();
    cout << (1e-9 * timer.elapsed().count()) << endl;

    for (auto i = 1; i < data.size(); ++i)
	if (data[i-1] > data[i])
	    cout << i << " " << data[i-1] << " " << data[i]  << endl;
    
    return 0;
}
