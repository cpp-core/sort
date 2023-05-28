// Copyright (C) 2022, 2023 by Mark Melton
//

#undef NDEBUG
#include <cassert>

#include <algorithm>
#include <iostream>
#include <random>
#include <span>

template<class T>
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
	    std::copy(other.data_, other.data_ + size_, data_);
	    return *this;
	}
	
	reference(value_type&);
	
	reference& operator=(const value_type&);
	
	friend void swap(reference a, reference b) {
	    assert(a.size() == b.size());
	    for (auto i = 0; i < a.size(); ++i)
		std::swap(a.data_[i], b.data_[i]);
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
	    : size_(r.size()) {
	    std::copy(r.data(), r.data() + r.size(), &data_[0]);
	}

	value_type(value_type&& other) noexcept
	    : size_(other.size()) {
	    std::copy(other.data(), other.data() + other.size(), &data_[0]);
	}

	storage_pointer data() const {
	    return (storage_pointer)&data_[0];
	}

	size_t size() const {
	    return size_;
	}

    private:
	storage_type data_[56];
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

template<class T>
RecordIterator<T>::reference::reference(RecordIterator<T>::value_type& value)
    : data_(value.data())
    , size_(value.size()) {
}

template<class T>
typename RecordIterator<T>::reference& RecordIterator<T>::reference::operator=
(const RecordIterator<T>::value_type& value) {
    assert(size() == value.size());
    std::copy(value.data(), value.data() + value.size(), data());
    return *this;
}

using std::cout, std::endl;

int main(int argc, const char *argv[]) {
    int nrecords = 10'000'000, nbytes = 50;
    std::vector<uint8_t> data(nrecords * nbytes);
    
    std::uniform_int_distribution<uint8_t> d;
    std::mt19937_64 rng;
    std::generate(data.begin(), data.end(), [&]() { return d(rng); });

    int key_index = 20;
    auto cmp = [&](RecordIterator<uint8_t>::reference a, RecordIterator<uint8_t>::reference b) {
	int aval = *(int*)(a.data() + key_index), bval = *(int*)(b.data() + key_index);
	return aval < bval;
    };

    RecordIterator begin(data.data(), nbytes);
    RecordIterator end(data.data() + data.size(), nbytes);
    std::sort(begin, end, cmp);

    int last_value = std::numeric_limits<int>::min();
    for (auto iter = RecordIterator(data.data(), nbytes);
	 iter != RecordIterator(data.data() + data.size(), nbytes);
	 ++iter) {
	auto a = (*iter).data();
	auto val = *(int*)(a + key_index);
	assert(last_value <= val);
	last_value = val;
    }

    return 0;
}
