// Copyright (C) 2022, 2023 by Mark Melton
//

#pragma once
#include <algorithm>
#include "frame.h"
#include "key.h"

namespace core::sort {

void merge_bottom_up(Frame& frame, const Keys& keys) {
    Frame buffer = frame;
    int n = frame.nrows();
    for (auto w = 1; w < n; w *= 2) {
	for (auto i = 0, mdx = 0; i < n; i += 2 * w) {
	    auto lptr = frame.row(i), rptr = frame.row(i + w);
	    auto elptr = frame.row(std::min(i + w, n)), erptr = frame.row(std::min(i + 2 * w, n));
	    while (lptr < elptr and rptr < erptr) {
		if (compare(lptr, rptr, keys) or not compare(rptr, lptr, keys)) {
		    std::copy(lptr, lptr + frame.bytes_per_row(), buffer.row(mdx));
		    lptr += frame.bytes_per_row();
		    ++mdx;
		} else {
		    std::copy(rptr, rptr + frame.bytes_per_row(), buffer.row(mdx));
		    rptr += frame.bytes_per_row();
		    ++mdx;
		}
	    }
	    while (lptr < elptr) {
		std::copy(lptr, lptr + frame.bytes_per_row(), buffer.row(mdx));
		lptr += frame.bytes_per_row();
		++mdx;
	    }
	    while (rptr < erptr) {
		std::copy(rptr, rptr + frame.bytes_per_row(), buffer.row(mdx));
		rptr += frame.bytes_per_row();
		++mdx;
	    }
	}
	std::swap(frame, buffer);
    }
}

}; // core::sort
