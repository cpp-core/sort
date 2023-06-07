// Copyright (C) 2022, 2023 by Mark Melton
//

#undef NDEBUG
#include <cassert>

#include <iostream>
#include <random>
#include <vector>
#include "core/record/iterator.h"
#include "boost/sort/sort.hpp"

using namespace core;
using std::cout, std::endl;

namespace std {
template<class T, bool A, bool B>
void swap(typename core::record::Iterator<T, A, B>::reference a,
	  typename core::record::Iterator<T, A, B>::reference b) {
    core::record::Iterator<T, A, B>::reference::swap(a, b);
}
};

void output_records(const std::vector<int>& data, int nrows, int ncols) {
    for (auto i = 0; i < nrows; ++i) {
	const int *row = &data[i * ncols];
	for (auto j = 0; j < ncols; ++j)
	    cout << row[j] << " ";
	cout << endl;
    }
}

int main(int argc, const char *argv[]) {
    // Generate 10k records each with 7 integer numbered sequentially.
    int nrows = 9, ncols = 5;
    std::vector<int> data(nrows * ncols);
    std::generate(data.begin(), data.end(), [n=0]() mutable { return n++; });
    cout << endl << "sequential: " << endl;
    output_records(data, nrows, ncols);

    // Shuffle the records
    std::shuffle(record::begin(data, ncols), record::end(data, ncols), std::mt19937_64{});
    cout << endl << "shuffled: " << endl;
    output_records(data, nrows, ncols);
    
    // Sort the records
    record::Iterator begin(data.data(), ncols);
    record::Iterator end(begin + nrows);

    using std::swap;
    swap(*begin, *end);
    boost::sort::parallel_stable_sort(begin, end,
	      [](const int *a, const int *b) {
		  return a[0] < b[0];
	      });
    cout << endl << "sorted: " << endl;
    output_records(data, nrows, ncols);
    
    return 0;
}
