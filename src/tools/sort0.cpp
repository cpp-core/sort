// Copyright (C) 2022, 2023 by Mark Melton
//

#undef NDEBUG
#include <cassert>

#include <iostream>
#include <random>
#include "core/chrono/stopwatch.h"
#include "core/sort/record_iterator.h"

using namespace core::sort;
using std::cout, std::endl;

template<class T>
struct Record;

template<class T>
struct RecordReference {
    explicit RecordReference(T* data, size_t size):
        _data(data), _size(size) {}
    
    RecordReference(Record<T>&);
    
    RecordReference(RecordReference const& rhs):
        _data(rhs.data()), _size(rhs.size()) {}
    
    /// Because this represents a reference, an assignment represents a copy of
    /// the referred-to value, not of the reference
    RecordReference& operator=(RecordReference const& rhs) {
        assert(size() == rhs.size());
        std::memcpy(data(), rhs.data(), sizeof(T) * size());
        return *this;
    }
    
    RecordReference& operator=(Record<T> const& rhs);
    
    /// Also `swap` swaps 'through' the reference
    friend void swap(RecordReference a, RecordReference b) {
        assert(a.size() == b.size());
	std::swap_ranges(a.data(), a.data() + a.size(), b.data());
        // size_t size = sizeof(T) * a.size();
        // auto* buffer = (T*)alloca(size);
        // std::memcpy(buffer, a.data(), size);
        // std::memcpy(a.data(), b.data(), size);
        // std::memcpy(b.data(), buffer, size);
    }
    
    T *data() const { return _data; }
    
    size_t size() const { return _size; }
    
private:
    T *_data;
    size_t _size;
};

template<class T>
struct Record {
    static constexpr size_t InlineSize = 1; 
    
    Record(RecordReference<T> ref): _size(ref.size()) {
        if (is_inline()) {
            std::memcpy(&inline_data, ref.data(), sizeof(T) * size());
        }
        else {
            heap_data = (T*)std::malloc(sizeof(T) * size());
            std::memcpy(heap_data, ref.data(), sizeof(T) * size());
        }
    }

    Record(Record&& rhs) noexcept: _size(rhs.size()) {
        if (is_inline()) {
            std::memcpy(&inline_data, &rhs.inline_data, sizeof(T) * size());
        }
        else {
            heap_data = rhs.heap_data;
            rhs.heap_data = nullptr;
        }
    }
    
    ~Record() {
        if (!is_inline()) {
            std::free(heap_data);
        }
    }
    
    T *data() { return (T*)((Record const*)this)->data(); }
    
    const T *data() const { return is_inline() ? inline_data : heap_data; }
    
    size_t size() const { return _size; }
    
private:
    bool is_inline() const { return _size <= InlineSize; }
    
    size_t _size;
    union {
        T inline_data[InlineSize];
        T *heap_data;
    };
};

/// After the definition of Record (see below):

template<class T>
RecordReference<T>::RecordReference(Record<T>& rhs):
    _data(rhs.data()), _size(rhs.size()) {}

template<class T>
RecordReference<T>& RecordReference<T>::operator=(Record<T> const& rhs) {
    assert(size() == rhs.size());
    std::memcpy(data(), rhs.data(), sizeof(T) * size());
    return *this;
}

template<class T>
struct MemoryIterator {
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = Record<T>;
    using pointer = void*;
    using reference = RecordReference<T>;

    MemoryIterator(T *ptr, size_t size)
        : ptr_(ptr)
        , size_(size) {
    }

    reference operator*() const {
        return reference(ptr_, size_);
    }

    reference operator[](size_t index) const {
        return *(*this + index);
    }

    MemoryIterator& operator++() {
        ptr_ += size_;
        return *this;
    }

    MemoryIterator& operator--() {
        ptr_ -= size_;
        return *this;
    }

    MemoryIterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    MemoryIterator operator--(int) {
        auto tmp = *this;
        --(*this);
        return tmp;
    }

    MemoryIterator& operator+=(size_t n) {
        ptr_ += n * size_;
        return *this;
    }

    MemoryIterator operator+(size_t n) const {
        auto r = *this;
        r += n;
        return r;
    }

    MemoryIterator& operator-=(size_t n) {
        ptr_ -= n * size_;
        return *this;
    }

    MemoryIterator operator-(size_t n) const {
        auto r = *this;
        r -= n;
        return r;
    }
    
    friend bool operator==(const MemoryIterator& a, const MemoryIterator& b) {
        assert(a.size_ == b.size_);
        return a.ptr_ == b.ptr_;
    }
    
    friend std::strong_ordering operator<=>(const MemoryIterator& a, const MemoryIterator& b) {
        assert(a.size_ == b.size_);
        return a.ptr_ <=> b.ptr_;
    }

    friend difference_type operator-(const MemoryIterator& a, const MemoryIterator& b) {
        assert(a.size_ == b.size_);
        return (a.ptr_ - b.ptr_) / a.size_;
    }

private:
    T* ptr_;
    size_t size_;
};

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
	auto val = *(uint8_t*)a;
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

template<class T, size_t N, size_t NumBytes = sizeof(T) * N>
void measure_direct() {
    measure("direct", 10'000'000, sizeof(T) * N, [](auto *data, int nr, int nb) {
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
    using RecordReference = typename RecordIterator<T, SwapRanges, HeapValueType>::reference;
    measure("record", 10'000'000, sizeof(T) * n, [&](auto *data, int nr, int nb) {
	assert(sizeof(T) * n == nb);
	RecordIterator<T, SwapRanges, HeapValueType> begin((T*)data, nb / sizeof(T));
	RecordIterator<T, SwapRanges, HeapValueType> end(begin + nr);
	std::sort(begin, end, [](RecordReference a, RecordReference b) {
	    return *a.data() < *b.data();
	});
    });
}

template<class T>
void measure_memory(int n) {
    using RecordReference = typename MemoryIterator<T>::reference;
    measure("memory", 10'000'000, sizeof(T) * n, [&](auto *data, int nr, int nb) {
	assert(sizeof(T) * n == nb);
	MemoryIterator<T> begin((T*)data, nb / sizeof(T));
	MemoryIterator<T> end(begin + nr);
	std::sort(begin, end, [](RecordReference a, RecordReference b) {
	    return *a.data() < *b.data();
	});
    });
}

int main(int argc, const char *argv[]) {
    // for (auto i = 1; i <= 8; ++i)
    // 	measure_memory<uint64_t>(i);
    // cout << endl;
    
    for (auto i = 1; i <= 8; ++i)
	measure_record<uint8_t, false, false>(8*i);
    cout << endl;

    for (auto i = 1; i <= 8; ++i)
	measure_record<uint8_t, false, true>(8*i);
    cout << endl;

    for (auto i = 1; i <= 8; ++i)
	measure_record<uint8_t, true, false>(8*i);
    cout << endl;

    for (auto i = 1; i <= 8; ++i)
	measure_record<uint8_t, true, true>(8*i);
    cout << endl;

    measure_direct<uint8_t, 1>();
    measure_direct<uint8_t, 2>();
    measure_direct<uint8_t, 3>();
    measure_direct<uint8_t, 4>();
    measure_direct<uint8_t, 5>();
    measure_direct<uint8_t, 6>();
    measure_direct<uint8_t, 7>();
    measure_direct<uint8_t, 8>();
    
    return 0;
}
