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
    
    struct iterator {
	using iterator_category = std::random_access_iterator_tag;
	using difference_type = std::ptrdiff_t;
	using value_type = const element_type*;
	using pointer = value_type*;
	using reference = value_type&;
	
	iterator(value_type ptr, size_t row_size)
	    : ptr_(ptr)
	    , row_size_(row_size) {
	}

	auto operator++() {
	    ptr_ += row_size_;
	    return *this;
	}

	auto operator++(int) {
	    iterator tmp = *this;
	    ++(*this);
	    return tmp;
	}

	reference operator*() {
	    return ptr_;
	}

	pointer operator->() {
	    return &ptr_;
	}
	
	friend bool operator==(iterator a, iterator b) {
	    return a.ptr_ == b.ptr_;
	}
	
    private:
	value_type ptr_;
	size_t row_size_;
    };

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

    auto row(size_t idx) {
	return storage_.data() + idx * bytes_per_row_;
    }

    auto row(size_t idx) const {
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

    iterator begin() const {
	return iterator{storage_.data(), bytes_per_row()};
    }
    
    iterator end() const {
	return iterator{storage_.data() + nrows() * bytes_per_row(), bytes_per_row()};
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
