// Copyright (C) 2022, 2023 by Mark Melton
//

#include <algorithm>
#include <random>
#include "core/util/tool.h"

struct Record;

struct RecordReference {
    explicit RecordReference(void* data, size_t element_size):
        _data(data), _size(element_size) {}
    
    RecordReference(Record&);
    
    RecordReference(RecordReference const& rhs):
        _data(rhs.data()), _size(rhs.size()) {}
    
    /// Because this represents a reference, an assignment represents a copy of
    /// the referred-to value, not of the reference
    RecordReference& operator=(RecordReference const& rhs) {
        assert(size() == rhs.size());
        std::memcpy(data(), rhs.data(), size());
        return *this;
    }
    
    RecordReference& operator=(Record const& rhs);
    
    /// Also `swap` swaps 'through' the reference
    friend void swap(RecordReference a, RecordReference b) {
        assert(a.size() == b.size());
        size_t size = a.size();
        // Perhaps don't use alloca if you're really serious
        // about standard conformance
        auto* buffer = (void*)alloca(size);
        std::memcpy(buffer, a.data(), size);
        std::memcpy(a.data(), b.data(), size);
        std::memcpy(b.data(), buffer, size);
    }
    
    void* data() const { return _data; }
    
    size_t size() const { return _size; }
    
private:
    void* _data;
    size_t _size;
};

struct Record {
    static constexpr size_t InlineSize = 56; 
    
    Record(RecordReference ref): _size(ref.size()) {
        if (is_inline()) {
            std::memcpy(&inline_data, ref.data(), size());
        }
        else {
            heap_data = std::malloc(size());
            std::memcpy(heap_data, ref.data(), size());
        }
    }

    Record(Record&& rhs) noexcept: _size(rhs.size()) {
        if (is_inline()) {
            std::memcpy(&inline_data, &rhs.inline_data, size());
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
    
    void* data() { return (void*)((Record const*)this)->data(); }
    
    void const* data() const { return is_inline() ? inline_data : heap_data; }
    
    size_t size() const { return _size; }
    
private:
    bool is_inline() const { return _size <= InlineSize; }
    
    size_t _size;
    union {
        char inline_data[InlineSize];
        void* heap_data;
    };
};

/// After the definition of Record (see below):

RecordReference::RecordReference(Record& rhs):
    _data(rhs.data()), _size(rhs.size()) {}

RecordReference& RecordReference::operator=(Record const& rhs) {
    assert(size() == rhs.size());
    std::memcpy(data(), rhs.data(), size());
    return *this;
}

struct MemoryIterator {
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = Record;
    using pointer = void*;
    using reference = RecordReference;

    MemoryIterator(pointer ptr, size_t size)
        : ptr_((char*)ptr)
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
    char* ptr_;
    size_t size_;
};

using std::cout, std::endl;

int tool_main(int argc, const char *argv[]) {
    int nrecords = 10'000'000, nbytes = 50;
    std::vector<uint8_t> data(nrecords * nbytes);
    
    std::uniform_int_distribution<uint8_t> d;
    std::mt19937_64 rng;
    std::generate(data.begin(), data.end(), [&]() { return d(rng); });

    int key_index = 20;
    auto cmp = [&](RecordReference a, RecordReference b) {
	int aval = *(int*)((char*)a.data() + key_index), bval = *(int*)((char*)b.data() + key_index);
	return aval < bval;
    };

    MemoryIterator begin(data.data(), nbytes);
    MemoryIterator end(data.data() + data.size(), nbytes);
    std::sort(begin, end, cmp);

    int last_value = std::numeric_limits<int>::min();
    for (auto iter = MemoryIterator(data.data(), nbytes);
	 iter != MemoryIterator(data.data() + data.size(), nbytes);
	 ++iter) {
	auto a = (char*)(*iter).data();
	auto val = *(int*)(a + key_index);
	assert(last_value <= val);
	last_value = val;
    }
    

    return 0;
}
