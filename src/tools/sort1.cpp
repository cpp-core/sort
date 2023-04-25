// Copyright (C) 2022, 2023 by Mark Melton
//

#include <random>
#include "core/util/tool.h"
#include "core/chrono/stopwatch.h"
#include "core/util/random.h"
#include "core/string/split.h"

enum class DataType { Signed64, Unsigned8, Unsigned16, Unsigned32, Unsigned64 };
using SortIndex = std::vector<uint>;

namespace core::str::detail {
template<>
struct lexical_cast_impl<DataType> {
    static DataType parse(std::string_view s) {
	using enum DataType;
	if (s == "i64") return Signed64;
	else if (s == "u8") return Unsigned8;
	else if (s == "u16") return Unsigned16;
	else if (s == "u32") return Unsigned32;
	else if (s == "u64") return Unsigned64;
	throw lexical_cast_error(s, "DataType");
    }
};
}; // core::str::detail

using ElementType = uint8_t;

class Frame {
public:
    using element_type = ElementType;
    
    struct iterator {
	using iterator_category = std::random_access_iterator_tag;
	using difference_type = std::ptrdiff_t;
	using value_type = element_type*;
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

    Frame(element_type *data, size_t bytes_per_row, size_t number_rows)
	: data_(data)
	, row_size_(bytes_per_row)
	, size_(number_rows) {
    }

    auto data() const {
	return data_;
    }

    auto row(size_t idx) const {
	return data_ + idx * row_size_;
    }

    auto operator[](size_t idx) const {
	return data_[idx];
    }

    auto operator[](size_t idx, size_t jdx) const {
	return *(data_ + idx * row_size_ + jdx);
    }

    iterator begin() const {
	return iterator{data_, row_size_};
    }
    
    iterator end() const {
	return iterator{data_ + row_size_ * size_, row_size_};
    }

    size_t row_size() const {
	return row_size_;
    }
    
    size_t size() const {
	return size_;
    }
    
private:
    element_type *data_;
    size_t row_size_, size_;
};

struct Key {
    DataType type;
    size_t offset;

    size_t length() const {
	switch (type) {
	case DataType::Signed64:
	    return 8;
	case DataType::Unsigned8:
	    return 1;
	case DataType::Unsigned16:
	    return 2;
	case DataType::Unsigned32:
	    return 4;
	case DataType::Unsigned64:
	    return 8;
	}
    }
};

using Keys = std::vector<Key>;

namespace core::str::detail {
template<>
struct lexical_cast_impl<Key> {
    static Key parse(std::string_view s) {
	auto fields = split(s, ":");
	if (fields.size() != 2)
	    throw lexical_cast_error(s, "Key");
	auto data_type = lexical_cast<DataType>(fields[0]);
	auto offset = lexical_cast<uint64_t>(fields[1]);
	return Key{data_type, offset};
    }
};
}; // core::str

bool compare(ElementType *a_ptr, ElementType *b_ptr, const Keys& sort_keys) {
    for (const auto& key : sort_keys) {
	switch (key.type) {
	    using enum DataType;
	case Unsigned8:
	    {
		auto a = *reinterpret_cast<uint8_t*>(a_ptr + key.offset);
		auto b = *reinterpret_cast<uint8_t*>(b_ptr + key.offset);
		if (a < b) return true;
		else if (a > b) return false;
	    }
	    break;
	    
	case Unsigned16:
	    {
		auto a = *reinterpret_cast<uint16_t*>(a_ptr + key.offset);
		auto b = *reinterpret_cast<uint16_t*>(b_ptr + key.offset);
		if (a < b) return true;
		else if (a > b) return false;
	    }
	    break;
	    
	case Unsigned32:
	    {
		auto a = *reinterpret_cast<uint32_t*>(a_ptr + key.offset);
		auto b = *reinterpret_cast<uint32_t*>(b_ptr + key.offset);
		if (a < b) return true;
		else if (a > b) return false;
	    }
	    break;
	    
	case Unsigned64:
	    {
		auto a = *reinterpret_cast<uint64_t*>(a_ptr + key.offset);
		auto b = *reinterpret_cast<uint64_t*>(b_ptr + key.offset);
		if (a < b) return true;
		else if (a > b) return false;
	    }
	    break;
	    
	case Signed64:
	    {
		auto a = *reinterpret_cast<int64_t*>(a_ptr + key.offset);
		auto b = *reinterpret_cast<int64_t*>(b_ptr + key.offset);
		if (a < b) return true;
		else if (a > b) return false;
	    }
	    break;
	}
    }
    return false;
}

auto pointer_sort(const Frame& frame, const Keys& sort_keys) {
    std::vector<ElementType*> ptrs;
    for (auto ptr : frame)
	ptrs.push_back(ptr);
    
    std::sort(ptrs.begin(), ptrs.end(), [&](ElementType *a_ptr, ElementType *b_ptr) {
	return compare(a_ptr, b_ptr, sort_keys);
    });
    
    return ptrs;
}

auto index_sort(const Frame& frame, const Keys& sort_keys) {
    SortIndex index(frame.size());
    for (auto i = 0; i < frame.size(); ++i)
	index[i] = i;

    auto ptr = frame.data();
    std::sort(index.begin(), index.end(), [&](int idx, int jdx) {
	return compare(ptr + idx * frame.row_size(), ptr + jdx * frame.row_size(), sort_keys);
    });

    return index;
}

size_t total_key_length(const Keys& keys) {
    size_t n{};
    for (const auto& key : keys)
	n += key.length();
    return n;
}

using RadixIndex = int;
constexpr auto RadixSize = 257;

auto radix_sort(const Frame& frame, const Keys& sort_keys) {
    Keys keys = sort_keys;
    std::reverse(keys.begin(), keys.end());
    
    const auto key_length = total_key_length(keys);
    RadixIndex buckets[key_length][RadixSize];
    memset(buckets, 0, key_length * RadixSize * sizeof(RadixIndex));

    for (auto row : frame) {
	auto bdx = 0;
	for (const auto& key : keys) {
	    auto field = row + key.offset;
	    switch (key.type) {
		using enum DataType;
	    case Unsigned8:
		++buckets[bdx++][1 + field[0]];
		break;
	    case Unsigned16:
		for (auto i = 0; i < 2; ++i)
		    ++buckets[bdx++][1 + field[i]];
		break;
	    case Unsigned32:
		for (auto i = 0; i < 4; ++i)
		    ++buckets[bdx++][1 + field[i]];
		break;
	    case Unsigned64:
		for (auto i = 0; i < 8; ++i)
		    ++buckets[bdx++][1 + field[i]];
		break;
	    case Signed64:
		for (auto i = 0; i < 8; ++i)
		    ++buckets[bdx++][1 + field[i]];
		break;
	    }
	}
    }

    for (auto i = 0; i < key_length; ++i) {
	auto *counts = buckets[i];
	for (auto j = 1; j < 257; ++j)
	    counts[j] += counts[j - 1];
    }

    SortIndex index(frame.size()), new_index(frame.size());
    for (auto i = 0; i < frame.size(); ++i)
	index[i] = i;

    auto bdx = 0;
    for (const auto& key : keys) {
	switch (key.type) {
	    using enum DataType;
	case Unsigned8:
	case Unsigned16:
	case Unsigned32:
	case Unsigned64:
	    for (auto i = 0; i < key.length(); ++i) {
		auto *counts = buckets[bdx++];
		auto offset = key.offset + i;
		for (auto j = 0; j < frame.size(); ++j) {
		    auto value = frame[index[j], offset];
		    auto& loc = counts[value];
		    new_index[loc] = index[j];
		    ++loc;
		}
		std::swap(index, new_index);
	    }
	    break;
	case Signed64:
	    for (auto i = 0; i < key.length(); ++i) {
		auto *counts = buckets[bdx++];
		auto offset = key.offset + i;
		for (auto j = 0; j < frame.size(); ++j) {
		    auto value = frame[index[j], offset];
		    auto& loc = counts[value];
		    new_index[loc] = index[j];
		    ++loc;
		}
		std::swap(index, new_index);
	    }
	    break;
	}
    }

    return index;
}

auto radix_sort_dup(const Frame& frame, const Keys& sort_keys) {
    Keys keys = sort_keys;
    std::reverse(keys.begin(), keys.end());
    
    const auto key_length = total_key_length(keys);
    RadixIndex buckets[key_length][RadixSize];
    memset(buckets, 0, key_length * RadixSize * sizeof(RadixIndex));

    std::vector<std::vector<uint8_t>> radix_values(key_length);
    for (auto i = 0; i < key_length; ++i)
	radix_values[i].resize(frame.size());

    auto idx = 0;
    for (auto row : frame) {
	auto bdx = 0;
	for (const auto& key : keys) {
	    auto field = row + key.offset;
	    switch (key.type) {
		using enum DataType;
	    case Unsigned8:
		radix_values[bdx][idx] = field[0];
		++buckets[bdx++][1 + field[0]];
		break;
	    case Unsigned16:
		for (auto i = 0; i < 2; ++i) {
		    radix_values[bdx][idx] = field[i];
		    ++buckets[bdx++][1 + field[i]];
		}
		break;
	    case Unsigned32:
		for (auto i = 0; i < 4; ++i) {
		    radix_values[bdx][idx] = field[i];
		    ++buckets[bdx++][1 + field[i]];
		}
		break;
	    case Unsigned64:
		for (auto i = 0; i < 8; ++i) {
		    radix_values[bdx][idx] = field[i];
		    ++buckets[bdx++][1 + field[i]];
		}
		break;
	    case Signed64:
		for (auto i = 0; i < 8; ++i) {
		    radix_values[bdx][idx] = field[i];
		    ++buckets[bdx++][1 + field[i]];
		}
		break;
	    }
	}
	++idx;
    }

    SortIndex index(frame.size()), new_index(frame.size());
    for (auto i = 0; i < frame.size(); ++i)
	index[i] = i;

    auto bdx = 0;
    for (const auto& key : keys) {
	switch (key.type) {
	    using enum DataType;
	case Unsigned8:
	case Unsigned16:
	case Unsigned32:
	case Unsigned64:
	    for (auto i = 0; i < key.length(); ++i) {
		auto *counts = buckets[bdx++];
		for (auto j = 1; j < 257; ++j)
		    counts[j] += counts[j - 1];
		auto *values = radix_values[i].data();
		for (auto j = 0; j < frame.size(); ++j) {
		    auto value = values[index[j]];
		    auto& loc = counts[value];
		    new_index[loc] = index[j];
		    ++loc;
		}
		std::swap(index, new_index);
	    }
	    break;
	case Signed64:
	    for (auto i = 0; i < key.length(); ++i) {
		auto *counts = buckets[bdx++];
		for (auto j = 1; j < 257; ++j)
		    counts[j] += counts[j - 1];
		auto *values = radix_values[i].data();
		for (auto j = 0; j < frame.size(); ++j) {
		    auto value = values[index[j]];
		    auto& loc = counts[value];
		    new_index[loc] = index[j];
		    ++loc;
		}
		std::swap(index, new_index);
	    }
	    break;
	}
    }

    return index;
}

bool check_sort(const std::vector<ElementType*>& ptrs, const Keys& sort_keys) {
    for (auto i = 1; i < ptrs.size(); ++i)
	if (not compare(ptrs[i-1], ptrs[i], sort_keys) and compare(ptrs[i], ptrs[i-1], sort_keys))
	    return false;
    return true;
}

bool check_sort(const Frame& frame, const SortIndex& index, const Keys& sort_keys) {
    auto ptr = *frame.begin();
    for (auto i = 1; i < index.size(); ++i)
	if (not compare(ptr + index[i-1] * frame.row_size(),
			ptr + index[i] * frame.row_size(),
			sort_keys) and
	    compare(ptr + index[i] * frame.row_size(),
		    ptr + index[i-1] * frame.row_size(),
		    sort_keys))
	    return false;
    return true;
}

bool check_sort(const Frame& frame, const Keys& sort_keys) {
    auto ptr = reinterpret_cast<uint64_t*>(frame.data());
    for (auto i = 1; i < frame.size(); ++i)
	if (not (ptr[i-1] < ptr[i]) and ptr[i] < ptr[i-1])
	    return false;
    return true;
}

int tool_main(int argc, const char *argv[]) {
    ArgParse opts
	(
	 argValue<'n'>("number-rows", (uint64_t)100, "Number of rows"),
	 argValue<'r'>("bytes-per-row", (uint64_t)64, "Bytes per row"),
	 argValues<'*', std::vector, Key>("keys", "Sort keys"),
	 argFlag<'v'>("verbose", "Verbose diagnostics")
	 );
    opts.parse(argc, argv);
    auto number_rows = opts.get<'n'>();
    auto bytes_per_row = opts.get<'r'>();
    auto sort_keys = opts.get<'*'>();
    auto verbose = opts.get<'v'>();

    if (bytes_per_row bitand 0x7)
	throw core::runtime_error("bytes-per-row must be a multple of 8: {}", bytes_per_row);

    if (sort_keys.size() == 0)
	throw core::runtime_error("At least one sort key must be specified");

    if (verbose)
	cout << fmt::format("creating random dataset with {} rows and {} bytes-per-row",
			    number_rows, bytes_per_row)
	     << endl;

    chron::StopWatch timer;
    timer.mark();
    
    auto number_elements = number_rows * bytes_per_row / sizeof(uint64_t);
    std::vector<uint64_t> raw_data(number_elements);
    std::uniform_int_distribution<uint64_t> d{};
    for (auto i = 0; i < number_elements; ++i)
	raw_data[i] = d(core::rng());

    if (verbose) {
	auto millis = timer.elapsed_duration<std::chrono::milliseconds>().count();
	cout << fmt::format("dataset created in {}ms", millis) << endl;
    }

    ElementType *data = reinterpret_cast<ElementType*>(raw_data.data());
    Frame frame{data, bytes_per_row, number_rows};

    timer.mark();
    auto ptrs = pointer_sort(frame, sort_keys);
    if (verbose) {
	auto millis = timer.elapsed_duration<std::chrono::milliseconds>().count();
	cout << fmt::format("pointer sort: {}ms", millis) << endl;
    }
    if (not check_sort(ptrs, sort_keys))
	throw core::runtime_error("pointer sort failed");

    timer.mark();
    auto index = index_sort(frame, sort_keys);
    if (verbose) {
	auto millis = timer.elapsed_duration<std::chrono::milliseconds>().count();
	cout << fmt::format("index sort: {}ms", millis) << endl;
    }
    if (not check_sort(frame, index, sort_keys))
	throw core::runtime_error("index sort failed");

    timer.mark();
    auto radix_index = radix_sort(frame, sort_keys);
    if (verbose) {
	auto millis = timer.elapsed_duration<std::chrono::milliseconds>().count();
	cout << fmt::format("radix sort: {}ms", millis) << endl;
    }
    
    if (not check_sort(frame, radix_index, sort_keys))
	throw core::runtime_error("radix sort failed");

    timer.mark();
    auto radix_dup_index = radix_sort_dup(frame, sort_keys);
    if (verbose) {
	auto millis = timer.elapsed_duration<std::chrono::milliseconds>().count();
	cout << fmt::format("radix dup sort: {}ms", millis) << endl;
    }
    
    if (not check_sort(frame, radix_dup_index, sort_keys))
	throw core::runtime_error("radix dup sort failed");

    if (frame.row_size() == 8 and sort_keys.size() == 1) {
	timer.mark();
	auto ptr = reinterpret_cast<uint64_t*>(frame.data());
	std::sort(ptr, ptr + frame.size(), [](const uint64_t& a, const uint64_t& b) {
	    return a < b;
	});
	if (verbose) {
	    auto millis = timer.elapsed_duration<std::chrono::milliseconds>().count();
	    cout << fmt::format("vanilla sort: {}ms", millis) << endl;
	}
	if (not check_sort(frame, sort_keys))
	    throw core::runtime_error("vanilla sort failed");
    }
	
    return 0;
}
