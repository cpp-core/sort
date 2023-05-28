// Copyright (C) 2022, 2023 by Mark Melton
//

#include <algorithm>
#include <iostream>
#include <random>
#include <span>

struct RecordIterator {
    using storage_type = uint8_t;
    using storage_pointer = storage_type*;

    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = storage_pointer;
    using pointer = storage_pointer;
    
    struct reference {
	reference(storage_pointer ptr, size_t record_size)
	    : ptr_(ptr)
	    , record_size_(record_size) {
	}
	
	operator uint8_t*() {
	    return ptr_;
	}
	
	auto& operator=(reference other) {
	    std::copy(other.ptr_, other.ptr_ + record_size_, ptr_);
	    return *this;
	}
	
	auto& operator=(uint8_t *ptr) {
	    std::copy(ptr, ptr + record_size_, ptr_);
	    return *this;
	}
	
	friend void swap(reference a, reference b) {
	    for (auto i = 0; i < a.record_size_; ++i)
		std::swap(a.ptr_[i], b.ptr_[i]);
	}
	
    private:
	storage_pointer ptr_;
	size_t record_size_;
    };
    
    RecordIterator(storage_pointer ptr, size_t record_size)
	: ptr_(ptr)
	, record_size_(record_size) {
    }

    reference operator*() {
	return {ptr_, record_size_};
    }

    reference operator[](size_t i) {
	return {ptr_ + i * record_size_, record_size_};
    }
    
    RecordIterator& operator++() {
	ptr_ += record_size_;
	return *this;
    }
    
    RecordIterator& operator--() {
	ptr_ -= record_size_;
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
	ptr_ += n * record_size_;
	return *this;
    }

    RecordIterator operator+(size_t n) {
	auto r = *this;
	r += n;
	return r;
    }

    RecordIterator& operator-=(size_t n) {
	ptr_ -= n * record_size_;
	return *this;
    }

    RecordIterator operator-(size_t n) {
	auto r = *this;
	r -= n;
	return r;
    }

    friend difference_type operator-(const RecordIterator& a, const RecordIterator& b) {
	return (a.ptr_ - b.ptr_) / a.record_size_;
    }

    friend auto operator<=>(const RecordIterator& a, const RecordIterator& b) = default;
    friend bool operator==(const RecordIterator& a, const RecordIterator& b) = default;

private:
    storage_pointer ptr_;
    size_t record_size_;
};

using std::cout, std::endl;

int main(int argc, const char *argv[]) {
    int nrecords = 20, nbytes = 16;
    std::vector<uint8_t> data(nrecords * nbytes);
    
    std::uniform_int_distribution<uint8_t> d;
    std::mt19937_64 rng;
    std::generate(data.begin(), data.end(), [&]() { return d(rng); });

    int key_index = 8;
    auto cmp = [&](uint8_t *a, uint8_t *b) {
	int aval = *(int*)(a + key_index), bval = *(int*)(b + key_index);
	return aval < bval;
    };

    RecordIterator begin(data.data(), nbytes);
    RecordIterator end(data.data() + data.size(), nbytes);
    std::sort(begin, end, cmp);

    return 0;
}
