// Copyright (C) 2022, 2023 by Mark Melton
//

#pragma once
#include <algorithm>
#include <latch>
#include <thread>

namespace core::sort {

template<class Iter, class Compare, size_t SampleBlock = 64>
void psort_sample(size_t nth, Iter begin, Iter end, Compare cmp) {
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
	
    std::vector<std::thread> threads;
    std::vector<size_t> prefix(nth);
    std::latch sync_copy(nth);
    for (auto tid = 0; tid < nth; ++tid) {
	threads.emplace_back([&,tid]() {
	    std::vector<value_type> tmp_data;
	    for (auto i = 0; i < ndata; ++i)
		if ((tid == 0 or begin[i] >= pivots[tid-1]) and
		    (tid == nth - 1 or begin[i] < pivots[tid]))
		    tmp_data.push_back(begin[i]);
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
