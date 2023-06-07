// Copyright (C) 2022, 2023 by Mark Melton
//

#include <iostream>
#include <random>
#include "core/timer/timer.h"
#include "core/sort/psort_sample.h"

#include <array>
#include <latch>

namespace core::sort {

template<class Iter, class Compare, size_t SampleBlock = 64>
void psort_sample_block(size_t nth, Iter begin, Iter end, Compare cmp) {
    if (nth == 1) {
	std::sort(begin, end, cmp);
	return;
    }

    using value_type = typename Iter::value_type;
    size_t ndata = end - begin;
    
    std::vector<value_type> sample;
    for (auto i = 0; i < ndata; i += SampleBlock)
	sample.push_back(begin[i]);
    std::sort(sample.begin(), sample.end(), cmp);
	
    std::vector<value_type> pivots;
    auto sample_ratio = (double)sample.size() / nth;
    for (auto i = 1; i < nth; ++i)
	pivots.push_back(sample[i * sample_ratio]);

    constexpr size_t BlockSize = 64;
    std::vector<std::thread> threads;
    std::vector<size_t> prefix(nth);
    std::latch sync_copy(nth);
    for (auto tid = 0; tid < nth; ++tid) {
	threads.emplace_back([&,tid]() {
	    std::array<unsigned char, BlockSize> offset;
	    std::vector<value_type> tmp_data;
	    auto min_value = tid == 0 ? 0 : pivots[tid - 1];
	    auto max_value = tid == nth - 1 ? uint64_t(-1) : pivots[tid];

	    auto ptr = &begin[0];
	    auto end_ptr = ptr + ndata;
	    auto end_ptr_block = end_ptr - (ndata % BlockSize);

	    while (ptr < end_ptr_block) {
		auto offset_ptr = ptr;
		
		size_t idx{};
		for (auto i = 0; i < BlockSize; ++i) {
		    offset[idx] = i;
		    idx += *ptr >= min_value and *ptr < max_value;
		    ++ptr;
		}

		for (auto i = 0; i < idx; ++i)
		    tmp_data.push_back(offset_ptr[offset[i]]);
	    }

	    while (ptr < end_ptr) {
		if (*ptr >= min_value and *ptr < max_value)
		    tmp_data.push_back(*ptr);
		++ptr;
	    }
	    
	    // for (auto i = 0; i < ndata; ++i)
	    // 	if (begin[i] >= min_value and begin[i] < max_value)
	    // 	    tmp_data.push_back(begin[i]);
	    
	    std::sort(tmp_data.begin(), tmp_data.end(), cmp);
	    prefix[tid] = tmp_data.size();
	    
	    sync_copy.arrive_and_wait();
	    size_t idx{};
	    for (auto i = 0; i < tid; ++i)
		idx += prefix[i];
	    std::copy(tmp_data.begin(), tmp_data.end(), &begin[idx]);
	});
    }

    for (auto& worker : threads)
	worker.join();
}

}; // core::sort


using std::cout, std::endl;
using namespace core;

int main(int argc, const char *argv[]) {
    size_t nworkers = argc < 2 ? 2 : atoi(argv[1]);
    size_t nrecords = argc < 3 ? 100'000'000 : atoi(argv[2]);
    
    std::uniform_int_distribution<uint64_t> d;
    std::mt19937_64 rng;
    std::vector<uint64_t> data(nrecords);
    std::generate(data.begin(), data.end(), [&]() { return d(rng); });

    timer::Timer timer;
    timer.start();
    sort::psort_sample_block(nworkers, data.begin(), data.end(), std::less{});
    timer.stop();
    cout << (1e-9 * timer.elapsed().count()) << endl;

    for (auto i = 1; i < data.size(); ++i)
	if (data[i-1] > data[i])
	    cout << i << " " << data[i-1] << " " << data[i]  << endl;
    
    return 0;
}
