// Copyright (C) 2022, 2023 by Mark Melton
//

#pragma once
#include <algorithm>

namespace core::sort {

template<class T, class Compare>
void bitonic_sort(T *data, size_t n) {
    for (auto k = 2; k <= n; k *= 2) {
	for (auto j = k/2; j > 0; j /= 2) {
	    for (auto i = 0; i < n; ++i) {
		auto l = i xor j;
		if (l > i) {
		    bool mask = i bitand k;
		    if ((not mask and (not Compare{}(data[i], data[l]))) or
			(mask and (Compare{}(data[i], data[l]))))
			std::swap(data[i], data[l]);
		}
	    }
	}
    }
}

}; // core::sort
