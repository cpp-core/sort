// Copyright (C) 2023 by Mark Melton
//

#pragma once
#include <iterator>
#include <algorithm>

namespace core::sort {

/// The `RecordIterator` template class implements the
/// `std::random_access_iterator` concept for a contiguous sequence of
/// equal sized records. This faciliates using generic algorithms,
/// such as `std::sort`, with run-time sized value types.
/// 
template<class T, size_t MaxRecordSize = std::numeric_limits<size_t>::max()>
struct RecordIterator {
    using storage_type = T;
    using storage_pointer = storage_type*;

    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;

    struct value_type;
    
    struct reference {
	explicit reference(storage_pointer data, size_t size)
	    : data_(data)
	    , size_(size) {
	}

	reference(const reference& other) = default;
	
	auto& operator=(reference other) {
	    assert(size() == other.size());
	    std::copy(other.data_, other.data_ + size_, data_);
	    return *this;
	}
	
	reference(value_type&);
	
	reference& operator=(const value_type&);
	
	friend void swap(reference a, reference b) {
	    assert(a.size() == b.size());
	    if constexpr (true or MaxRecordSize <= 32) {
		std::swap_ranges(a.data(), a.data() + a.size(), b.data());
	    } else {
		auto size = a.size();
		storage_pointer buffer = (storage_pointer)alloca(size * sizeof(storage_type));
		std::copy(a.data(), a.data() + size, buffer);
		std::copy(b.data(), b.data() + size, a.data());
		std::copy(buffer, buffer + size, b.data());
	    }
	}

	storage_pointer data() const {
	    return data_;
	}

	size_t size() const {
	    return size_;
	}
	
    private:
	storage_pointer data_;
	size_t size_;
    };

    struct value_type {
	value_type(reference r)
	    : data_((storage_pointer)std::malloc(r.size())),
	      size_(r.size()) {
	    std::copy(r.data(), r.data() + r.size(), data_);
	}

	value_type(value_type&& other) noexcept
	    : data_(other.data_)
	    , size_(other.size()) {
	    other.data_ = nullptr;
	}

	~value_type() {
	    free(data_);
	}

	storage_pointer data() const {
	    return (storage_pointer)&data_[0];
	}

	size_t size() const {
	    return size_;
	}

    private:
	storage_pointer data_{nullptr};
	size_t size_;
    };

    RecordIterator(storage_pointer data, size_t size)
	: data_(data)
	, size_(size) {
    }

    storage_pointer data() {
	return data_;
    }

    const storage_pointer data() const {
	return data_;
    }

    size_t size() const {
	return size_;
    }

    reference operator*() {
	return reference{data_, size_};
    }

    reference operator[](size_t i) const {
	return *(*this + index);
    }
    
    RecordIterator& operator++() {
	data_ += size_;
	return *this;
    }
    
    RecordIterator& operator--() {
	data_ -= size_;
	return *this;
    }
    
    RecordIterator operator++(int) {
	auto tmp = *this;
	++(*this);
	return tmp;
    }

    RecordIterator operator--(int) {
	auto tmp = *this;
	--(*this);
	return tmp;
    }

    RecordIterator& operator+=(size_t n) {
	data_ += n * size_;
	return *this;
    }

    RecordIterator operator+(size_t n) {
	auto r = *this;
	r += n;
	return r;
    }

    RecordIterator& operator-=(size_t n) {
	data_ -= n * size_;
	return *this;
    }

    RecordIterator operator-(size_t n) {
	auto r = *this;
	r -= n;
	return r;
    }

    friend difference_type operator-(const RecordIterator& a, const RecordIterator& b) {
	assert(a.size() == b.size());
	return (a.data_ - b.data_) / a.size_;
    }

    friend auto operator<=>(const RecordIterator& a, const RecordIterator& b) = default;
    friend bool operator==(const RecordIterator& a, const RecordIterator& b) = default;

private:
    storage_pointer data_;
    size_t size_;
};

template<class T, size_t N>
RecordIterator<T, N>::reference::reference(RecordIterator<T, N>::value_type& value)
    : data_(value.data())
    , size_(value.size()) {
}

template<class T, size_t N>
typename RecordIterator<T, N>::reference& RecordIterator<T, N>::reference::operator=
(const RecordIterator<T, N>::value_type& value) {
    assert(size() == value.size());
    std::copy(value.data(), value.data() + value.size(), data());
    return *this;
}

}; // core::sort
