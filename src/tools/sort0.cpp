// Copyright (C) 2022, 2023 by Mark Melton
//

#undef NDEBUG
#include <cassert>

#include <algorithm>
#include <iostream>
#include <random>
#include <span>
#include "core/chrono/stopwatch.h"

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
	    assert(size() == other.size());
	    std::copy(other.data_, other.data_ + size_, data_);
	    return *this;
	}
	
	reference(value_type&);
	
	reference& operator=(const value_type&);
	
	friend void swap(reference a, reference b) {
	    assert(a.size() == b.size());
	    // for (auto i = 0; i < a.size(); ++i)
	    // 	std::swap(a.data_[i], b.data_[i]);
	    auto size = a.size();
	    storage_pointer buffer = (storage_pointer)alloca(size * sizeof(storage_type));
	    std::copy(a.data(), a.data() + size, buffer);
	    std::copy(b.data(), b.data() + size, a.data());
	    std::copy(buffer, buffer + size, b.data());
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

template<class Work>
void measure(std::string_view desc, size_t nr, size_t nb, Work&& work) {
    std::vector<uint8_t> data(nr * nb);
    std::uniform_int_distribution<uint8_t> d;
    std::mt19937_64 rng;
    std::generate(data.begin(), data.end(), [&]() { return d(rng); });
    
    chron::StopWatch timer;
    timer.mark();
    work(data.data(), nr, nb);
    auto ms = timer.elapsed_duration<std::chrono::milliseconds>().count();
    cout << desc << " " << nr << " x " << nb << " : " << ms << " ms" << endl;

    uint64_t last_value{};
    for (auto iter = RecordIterator(data.data(), nb);
	 iter != RecordIterator(data.data() + data.size(), nb);
	 ++iter) {
	auto a = iter.data();
	auto val = *(uint64_t*)a;
	assert(last_value <= val);
	last_value = val;
    }
}

// M1
// direct 10000000 x 8 : 622 ms
// direct 10000000 x 64 : 759 ms
// direct 10000000 x 256 : 1362 ms
// direct 10000000 x 1024 : 5749 ms

// record 10000000 x 8 : 1196 ms
// record 10000000 x 64 : 1225 ms
// record 10000000 x 256 : 1730 ms
// record 10000000 x 1024 : 6219 ms

int main(int argc, const char *argv[]) {
    measure("direct", 10'000'000, 8, [](auto *data, int nr, int nb) {
	auto begin = (uint64_t*)data;
	auto end = (uint64_t*)((char*)data + nr * nb);
	std::sort(begin, end, [](auto a, auto b) {
	    return a < b;
	});
    });
    
    measure("direct", 10'000'000, 64, [](auto *data, int nr, int nb) {
	assert(nb == 64);
	struct s64 { uint64_t data[8]; };
	static_assert(sizeof(s64) == 64);
	auto begin = (s64*)data;
	auto end = begin + nr;
	std::sort(begin, end, [](auto a, auto b) {
	    return a.data[0] < b.data[0];
	});
    });
    
    measure("direct", 10'000'000, 256, [](auto *data, int nr, int nb) {
	assert(nb == 256);
	struct s256 { uint64_t data[32]; };
	static_assert(sizeof(s256) == 256);
	auto begin = (s256*)data;
	auto end = begin + nr;
	std::sort(begin, end, [](auto a, auto b) {
	    return a.data[0] < b.data[0];
	});
    });
    
    measure("direct", 10'000'000, 1024, [](auto *data, int nr, int nb) {
	assert(nb == 1024);
	struct s1024 { uint64_t data[128]; };
	static_assert(sizeof(s1024) == 1024);
	auto begin = (s1024*)data;
	auto end = begin + nr;
	std::sort(begin, end, [](auto a, auto b) {
	    return a.data[0] < b.data[0];
	});
    });
    
    cout << endl;
    
    measure("record", 10'000'000, 8, [](auto *data, int nr, int nb) {
	RecordIterator begin((uint8_t*)data, nb);
	RecordIterator end(begin + nr);
	std::sort(begin, end,
		  [](RecordIterator<uint8_t>::reference a, RecordIterator<uint8_t>::reference b) {
		      auto aval = *(uint64_t*)a.data(), bval = *(uint64_t*)b.data();
		      return aval < bval;
		  });
    });

    measure("record", 10'000'000, 64, [](auto *data, int nr, int nb) {
	RecordIterator begin((uint8_t*)data, nb);
	RecordIterator end(begin + nr);
	std::sort(begin, end,
		  [](RecordIterator<uint8_t>::reference a, RecordIterator<uint8_t>::reference b) {
		      auto aval = *(uint64_t*)a.data(), bval = *(uint64_t*)b.data();
		      return aval < bval;
		  });
    });
    
    measure("record", 10'000'000, 256, [](auto *data, int nr, int nb) {
	RecordIterator begin((uint8_t*)data, nb);
	RecordIterator end(begin + nr);
	std::sort(begin, end,
		  [](RecordIterator<uint8_t>::reference a, RecordIterator<uint8_t>::reference b) {
		      auto aval = *(uint64_t*)a.data(), bval = *(uint64_t*)b.data();
		      return aval < bval;
		  });
    });

    measure("record", 10'000'000, 1024, [](auto *data, int nr, int nb) {
	RecordIterator begin((uint8_t*)data, nb);
	RecordIterator end(begin + nr);
	std::sort(begin, end,
		  [](RecordIterator<uint8_t>::reference a, RecordIterator<uint8_t>::reference b) {
		      auto aval = *(uint64_t*)a.data(), bval = *(uint64_t*)b.data();
		      return aval < bval;
		  });
    });

    return 0;
}
