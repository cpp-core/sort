// Copyright (C) 2019, 2023 by Mark Melton
//

#include <random>
#include <vector>
#include <gtest/gtest.h>
#include "core/sort/record_iterator.h"

using namespace core::sort;

template<class T>
auto generate_random_data(int nrows, int ncols) {
    std::vector<T> data(nrows * ncols);
    std::uniform_int_distribution<T> d;
    std::mt19937_64 rng;
    std::generate(data.begin(), data.end(), [&]() { return d(rng); });
    return data;
}

template<class T>
auto generate_sequence_data(int nrows, int ncols) {
    std::vector<T> data(nrows * ncols);
    std::generate(data.begin(), data.end(), [n=0]() mutable { return n++; });
    return data;
}

template<class T>
void check_order(const std::vector<T>& data, size_t ncols) {
    auto last_value = std::numeric_limits<T>::min();
    for (auto iter = begin_record(data, ncols); iter != end_record(data, ncols);  ++iter) {
	auto value = *iter.data();
	EXPECT_GE(value, last_value);
	last_value = value;
    }
}

template<class T>
void check_sequence(const std::vector<T>& data, size_t ncols) {
    size_t count{};
    for (auto iter = begin_record(data, ncols); iter != end_record(data, ncols);  ++iter) {
	auto value = *iter.data();
	EXPECT_GE(value, count);
	count += ncols;
    }
}

TEST(RecordIterator, NoRows)
{
    int nrows = 0, ncols = 8;
    auto data = generate_sequence_data<int>(nrows, ncols);
    RecordIterator b = begin_record(data, ncols);
    RecordIterator e = end_record(data, ncols);
    EXPECT_EQ(e - b, nrows);
    EXPECT_EQ(b, e);
}

TEST(RecordIterator, Iterate)
{
    int nrows = 10, ncols = 8;
    auto data = generate_sequence_data<int>(nrows, ncols);
    EXPECT_EQ(end_record(data, ncols) - begin_record(data, ncols), nrows);
    check_sequence(data, ncols);
}

TEST(RecordIterator, Sort)
{
    int nrows = 1000, ncols = 8;
    auto data = generate_sequence_data<int>(nrows, ncols);
    
    std::shuffle(begin_record(data, ncols), end_record(data, ncols), std::mt19937_64{});
    std::sort(begin_record(data, ncols), end_record(data, ncols),
	      [](const auto& a, const auto& b) {
		  return *a.data() < *b.data();
	      });
    check_order(data, ncols);
    check_sequence(data, ncols);
}

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
