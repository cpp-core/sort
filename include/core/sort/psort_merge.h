// Copyright (C) 2022, 2023 by Mark Melton
//

#pragma once
#include <algorithm>
#include <thread>
#include <barrier>
#include <vector>

namespace core::sort {

template<class Iter, class Compare>
void psort_merge(size_t nth, Iter begin, Iter end, Compare cmp) {
    using value_type = typename Iter::value_type;

    size_t ndata = end - begin;
    std::vector<value_type> tmp_buffer(ndata);
    Iter tmp_iter = tmp_buffer.begin();

    size_t bucketsize = ndata / nth;
    std::barrier sync(nth);
    std::vector<std::thread> workers;
    for (auto tid = 0; tid < nth; ++tid) {
	workers.emplace_back([&,tid]() {
	    size_t sdx = tid * bucketsize;
	    size_t edx = std::min(sdx + bucketsize, ndata);
	    std::sort(begin + sdx, begin + edx, cmp);

	    for (size_t span = 2; span <= nth; span *= 2) {
		sync.arrive_and_wait();
		size_t gid = tid bitand ~(span - 1);
		size_t sdx = gid * bucketsize;
		size_t mdx = sdx + (span / 2) * bucketsize;
		size_t edx = std::min(sdx + span * bucketsize, ndata);

		if (tid % span == 0) {
		    size_t idx{sdx}, ldx{sdx}, rdx{mdx};
		    while (idx < mdx and ldx < mdx and rdx < edx) {
			if (begin[ldx] < begin[rdx])
			    tmp_iter[idx++] = begin[ldx++];
			else
			    tmp_iter[idx++] = begin[rdx++];
		    }
		} else if (tid % span == 1) {
		    ssize_t idx(edx - 1);
		    ssize_t rdx{idx};
		    ssize_t ldx(mdx - 1);
		    while (idx >= mdx and ldx >= sdx and rdx >= mdx) {
			if (begin[ldx] > begin[rdx])
			    tmp_iter[idx--] = begin[ldx--];
			else
			    tmp_iter[idx--] = begin[rdx--];
		    }
		}
		sync.arrive_and_wait();
		if (tid == 0)
		    std::swap(begin, tmp_iter);
	    }
	});
    }

    for (auto& worker : workers)
	worker.join();

    if (nth == 2 or nth == 8 or nth == 32)
	std::copy(begin, begin + ndata, tmp_iter);
}

}; // core::sort
