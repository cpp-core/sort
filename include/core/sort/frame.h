// Copyright (C) 2022, 2023 by Mark Melton
//

#pragma once
#include <span>
#include <fmt/format.h>
#include "core/util/random.h"
#include "type.h"

namespace core::sort {

using ElementType = uint8_t;

class Frame {
public:
    using element_type = ElementType;
    
    Frame(size_t number_rows, size_t bytes_per_row)
	: storage_(number_rows * bytes_per_row)
	, nrows_(number_rows)
	, bytes_per_row_(bytes_per_row) {
	std::uniform_int_distribution<uint8_t> d{};
	for (auto i = 0; i < nrows_ * bytes_per_row_; ++i)
	    storage_[i] = d(core::rng());
    }

    Frame clone() const {
	return Frame(*this);
    }

    Frame order_by(const std::vector<int>& index) const {
	Frame copy = clone();
	for (auto i = 0; i < index.size(); ++i)
	    std::copy(row(index[i]), row(index[i] + 1), copy.row(i));
	return copy;
    }

    auto nrows() const {
	return nrows_;
    }

    auto bytes_per_row() const {
	return bytes_per_row_;
    }

    auto data() {
	return storage_.data();
    }

    auto data() const {
	return storage_.data();
    }

    auto *begin() {
	return storage_.data();
    }

    const auto *begin() const {
	return storage_.data();
    }

    auto *end() {
	return storage_.data() + nrows() * bytes_per_row();
    }

    const auto *end() const {
	return storage_.data() + nrows() * bytes_per_row();
    }

    element_type *row(size_t idx) {
	return storage_.data() + idx * bytes_per_row_;
    }

    const element_type *row(size_t idx) const {
	return storage_.data() + idx * bytes_per_row_;
    }

    void swap_rows(size_t idx, size_t jdx) {
	std::swap_ranges(row(idx), row(idx+1), row(jdx));
    }

    auto operator[](size_t idx) const {
	return storage_[idx];
    }

    auto operator[](size_t idx, size_t jdx) const {
	return *(data() + idx * bytes_per_row_ + jdx);
    }

private:
    std::vector<ElementType> storage_;
    size_t nrows_, bytes_per_row_;
};

std::ostream& operator<<(std::ostream& os, const Frame& frame) {
    for (auto i = 0; i < frame.nrows(); ++i) {
	for (auto j = 0; j < frame.bytes_per_row(); ++j)
	    os << fmt::format("{:02x} ", frame[i, j]);
	os << std::endl;
    }
    return os;
}

}; // core::sort
