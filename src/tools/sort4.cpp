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

    core::timer::Timer timer;
    timer.start();

    if (nworkers == 1) {
	std::sort(data.begin(), data.end());
    } else {
	std::vector<uint64_t> sample;
	constexpr size_t blocksize = 64;
	for (auto i = 0; i < data.size(); i += blocksize)
	    sample.push_back(data[i]);
	std::sort(sample.begin(), sample.end());
	
	std::vector<uint64_t> pivots;
	auto sample_ratio = (double)sample.size() / nworkers;
	for (auto i = 1; i < nworkers; ++i)
	    pivots.push_back(sample[i * sample_ratio]);
	
	std::vector<std::thread> workers;
	std::vector<size_t> prefix(nworkers);
	std::latch sync_before_prefix(nworkers), sync_before_copy(nworkers);
	for (auto wid = 0; wid < nworkers; ++wid) {
	    workers.emplace_back([&,wid]() {
		std::vector<uint64_t> tmp_data;
		if (wid == 0) {
		    for (auto i = 0; i < data.size(); ++i)
			if (data[i] < pivots[wid])
			    tmp_data.push_back(data[i]);
		} else if (wid == nworkers - 1) {
		    for (auto i = 0; i < data.size(); ++i)
			if (data[i] >= pivots[wid-1])
			    tmp_data.push_back(data[i]);
		} else {
		    for (auto i = 0; i < data.size(); ++i)
			if (data[i] >= pivots[wid-1] and data[i] < pivots[wid])
			    tmp_data.push_back(data[i]);
		}
		std::sort(tmp_data.begin(), tmp_data.end());
		
		sync_before_prefix.arrive_and_wait();
		prefix[wid] = tmp_data.size();
		
		sync_before_copy.arrive_and_wait();
		size_t idx{};
		for (auto i = 0; i < wid; ++i)
		    idx += prefix[i];
		std::copy(tmp_data.begin(), tmp_data.end(), &data[idx]);
	    });
	}

	for (auto& worker : workers)
	    worker.join();
    }

    timer.stop();
    cout << (1e-9 * timer.elapsed().count()) << endl;

    for (auto i = 1; i < data.size(); ++i)
	if (data[i-1] > data[i])
	    cout << i << " " << data[i-1] << " " << data[i]  << endl;
    
    return 0;
}
