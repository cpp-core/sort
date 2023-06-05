// Copyright (C) 2022, 2023 by Mark Melton
//

#undef NDEBUG
#include <cassert>

#include <algorithm>
#include <iostream>
#include <random>
#include <thread>
#include <barrier>
#include "core/timer/timer.h"

using std::cout, std::endl;

int main(int argc, const char *argv[]) {
    size_t nth = argc < 2 ? 2 : atoi(argv[1]);
    if (std::popcount(nth) != 1) {
	cout << "Number of threads must be power of two: " << nth << endl;
	return -1;
    }
    
    int nrecords = 100'000'000;
    std::vector<uint64_t> data(nrecords);
    
    std::uniform_int_distribution<uint64_t> d;
    std::mt19937_64 rng;
    std::generate(data.begin(), data.end(), [&]() { return d(rng); });

    std::vector<std::thread> workers;
    
    core::timer::Timer timer;
    timer.start();

    std::vector<uint64_t> result(data.size());
    size_t bucketsize = data.size() / nth;
    std::barrier sync(nth);
    for (auto tid = 0; tid < nth; ++tid) {
	workers.emplace_back([&,tid]() {
	    size_t sdx = tid * bucketsize;
	    size_t edx = std::min(sdx + bucketsize, data.size());
	    std::sort(&data[sdx], &data[edx]);

	    for (size_t span = 2; span <= nth; span *= 2) {
		sync.arrive_and_wait();
		size_t gid = tid bitand ~(span - 1);
		size_t sdx = gid * bucketsize;
		size_t mdx = sdx + (span / 2) * bucketsize;
		size_t edx = std::min(sdx + span * bucketsize, data.size());

		cout << "t: " << tid << " " << gid << " " << sdx << " " << mdx << " " << edx << endl;
		
		if (tid % span == 0) {
		    size_t idx{sdx}, ldx{sdx}, rdx{mdx};
		    cout << "l: " << idx << " " << ldx << " " << rdx << endl;
		    while (idx < mdx and ldx < mdx and rdx < edx) {
			if (data[ldx] < data[rdx])
			    result[idx++] = data[ldx++];
			else
			    result[idx++] = data[rdx++];
		    }
		} else if (tid % span == 1) {
		    ssize_t idx(edx - 1);
		    ssize_t rdx{idx};
		    ssize_t ldx(mdx - 1);
		    cout << "r: " << idx << " " << ldx << " " << rdx << endl;
		    while (idx >= mdx and ldx >= sdx and rdx >= mdx) {
			if (data[ldx] > data[rdx])
			    result[idx--] = data[ldx--];
			else
			    result[idx--] = data[rdx--];
		    }
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

    for (auto i = 49'999'990; i < 50'000'010; ++i)
	cout << data[i] << endl;
    
    return 0;
}
