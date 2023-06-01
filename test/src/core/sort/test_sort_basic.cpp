// Copyright (C) 2019, 2023 by Mark Melton
//

#include <random>
#include <vector>
#include <gtest/gtest.h>
#include "core/record/record.h"

using namespace core;

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
    for (auto iter = record::begin(data, ncols); iter != record::end(data, ncols);  ++iter) {
	auto value = *iter.data();
	EXPECT_GE(value, last_value);
	last_value = value;
    }
}

template<class T>
void check_sequence(const std::vector<T>& data, size_t ncols) {
    size_t count{};
    for (auto iter = record::begin(data, ncols); iter != record::end(data, ncols);  ++iter) {
	auto value = *iter.data();
	EXPECT_GE(value, count);
	count += ncols;
    }
}

TEST(Sort, NoRows)
{
    int nrows = 0, ncols = 8;
    auto data = generate_sequence_data<int>(nrows, ncols);
    auto b = record::begin(data, ncols);
    auto e = record::end(data, ncols);
    EXPECT_EQ(e - b, nrows);
    EXPECT_EQ(b, e);
}

TEST(Sort, Iterate)
{
    int nrows = 10, ncols = 8;
    auto data = generate_sequence_data<int>(nrows, ncols);
    EXPECT_EQ(record::end(data, ncols) - record::begin(data, ncols), nrows);
    check_sequence(data, ncols);
}

TEST(Sort, Sort)
{
    int nrows = 1000, ncols = 8;
    auto data = generate_sequence_data<int>(nrows, ncols);
    
    std::shuffle(record::begin(data, ncols), record::end(data, ncols), std::mt19937_64{});
    std::sort(record::begin(data, ncols), record::end(data, ncols),
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
