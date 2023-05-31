// Copyright (C) 2022, 2023 by Mark Melton
//

#undef NDEBUG
#include <cassert>

#include <iostream>
#include <random>
#include <sstream>
#include "core/sort/record_iterator.h"
#include "core/timer/timer.h"

using namespace core;
using std::cout, std::endl;

template<class Work>
void measure(std::string_view desc, size_t nr, size_t nb, Work&& work) {
    std::vector<uint8_t> data(nr * nb);
    std::uniform_int_distribution<uint8_t> dist;
    std::mt19937_64 rng;
    std::generate(data.begin(), data.end(), [&]() { return dist(rng); });

    auto d = timer::Timer().run(1, [&]() {
	work(data.data(), nr, nb);
    }).elapsed();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
    cout << desc << " " << nr << " x " << nb << " : " << ms << " ms" << endl;
    
    uint64_t last_value{};
    for (auto iter = sort::begin_record(data, nb); iter != sort::end_record(data, nb); ++iter) {
	auto a = iter.data();
	auto val = *(uint64_t*)a;
	assert(last_value <= val);
	last_value = val;
    }
}

// M1
// memory 10000000 x 8 : 751 ms
// memory 10000000 x 16 : 772 ms
// memory 10000000 x 24 : 793 ms
// memory 10000000 x 32 : 815 ms
// memory 10000000 x 40 : 840 ms
// memory 10000000 x 48 : 868 ms
// memory 10000000 x 56 : 889 ms
// memory 10000000 x 64 : 916 ms

// record.8.0.0 10000000 x 8 : 1166 ms
// record.8.0.0 10000000 x 16 : 1316 ms
// record.8.0.0 10000000 x 24 : 1368 ms
// record.8.0.0 10000000 x 32 : 1431 ms
// record.8.0.0 10000000 x 40 : 1485 ms
// record.8.0.0 10000000 x 48 : 1551 ms
// record.8.0.0 10000000 x 56 : 1594 ms
// record.8.0.0 10000000 x 64 : 1194 ms

// record.8.0.1 10000000 x 8 : 1182 ms
// record.8.0.1 10000000 x 16 : 1329 ms
// record.8.0.1 10000000 x 24 : 1384 ms
// record.8.0.1 10000000 x 32 : 1454 ms
// record.8.0.1 10000000 x 40 : 1510 ms
// record.8.0.1 10000000 x 48 : 1563 ms
// record.8.0.1 10000000 x 56 : 1604 ms
// record.8.0.1 10000000 x 64 : 1229 ms

// record.8.1.0 10000000 x 8 : 779 ms
// record.8.1.0 10000000 x 16 : 796 ms
// record.8.1.0 10000000 x 24 : 817 ms
// record.8.1.0 10000000 x 32 : 829 ms
// record.8.1.0 10000000 x 40 : 849 ms
// record.8.1.0 10000000 x 48 : 869 ms
// record.8.1.0 10000000 x 56 : 896 ms
// record.8.1.0 10000000 x 64 : 910 ms

// record.8.1.1 10000000 x 8 : 772 ms
// record.8.1.1 10000000 x 16 : 797 ms
// record.8.1.1 10000000 x 24 : 825 ms
// record.8.1.1 10000000 x 32 : 853 ms
// record.8.1.1 10000000 x 40 : 869 ms
// record.8.1.1 10000000 x 48 : 895 ms
// record.8.1.1 10000000 x 56 : 917 ms
// record.8.1.1 10000000 x 64 : 943 ms

// direct 10000000 x 8 : 620 ms
// direct 10000000 x 16 : 627 ms
// direct 10000000 x 24 : 668 ms
// direct 10000000 x 32 : 668 ms
// direct 10000000 x 40 : 698 ms
// direct 10000000 x 48 : 711 ms
// direct 10000000 x 56 : 730 ms
// direct 10000000 x 64 : 749 ms

template<class T, size_t N, size_t NumBytes = sizeof(T) * N>
void measure_direct() {
    measure("direct", 1'000'000, sizeof(T) * N, [](auto *data, int nr, int nb) {
	assert(nb == NumBytes);
	struct sN { T data[N]; };
	auto begin = (sN*)data;
	auto end = (sN*)((char*)data + nr * nb);
	std::sort(begin, end, [](auto a, auto b) {
	    return a.data[0] < b.data[0];
	});
    });
}

template<class T, bool SwapRanges = false, bool HeapValueType = true>
void measure_record(int n) {
    std::stringstream ss;
    ss << "record" << "." << sizeof(T) << "." << SwapRanges << "." << HeapValueType;
    measure(ss.str(), 1'000'000, sizeof(T) * n, [&](auto *data, int nr, int nb) {
	assert(sizeof(T) * n == nb);
	sort::RecordIterator<T, SwapRanges, HeapValueType> begin((T*)data, nb / sizeof(T));
	sort::RecordIterator<T, SwapRanges, HeapValueType> end(begin + nr);
	std::sort(begin, end, [](const auto& a, const auto& b) {
	    return *a.data() < *b.data();
	});
    });
}

int main(int argc, const char *argv[]) {
    for (auto i = 1; i <= 8; ++i)
    	measure_record<uint64_t, false, false>(i);
    cout << endl;

    for (auto i = 1; i <= 8; ++i)
    	measure_record<uint64_t, false, true>(i);
    cout << endl;

    for (auto i = 1; i <= 8; ++i)
    	measure_record<uint64_t, true, false>(i);
    cout << endl;

    for (auto i = 1; i <= 8; ++i)
    	measure_record<uint64_t, true, true>(i);
    cout << endl;

    measure_direct<uint64_t, 1>();
    measure_direct<uint64_t, 2>();
    measure_direct<uint64_t, 3>();
    measure_direct<uint64_t, 4>();
    measure_direct<uint64_t, 5>();
    measure_direct<uint64_t, 6>();
    measure_direct<uint64_t, 7>();
    measure_direct<uint64_t, 8>();
    
    return 0;
}
