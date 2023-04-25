// Copyright (C) 2022, 2023 by Mark Melton
//

#pragma once
#include "core/string/split.h"
#include "type.h"

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

std::ostream& operator<<(std::ostream& os, const Key& key) {
    os << key.type << ":" << (8 * key.length());
    return os;
}

std::ostream& operator<<(std::ostream& os, const Keys& keys) {
    os << "[ ";
    for (const auto& key : keys)
	os << key << " ";
    os << "]";
    return os;
}

template<class T>
bool compare(const T *a_ptr, const T *b_ptr, const Keys& sort_keys) {
    for (const auto& key : sort_keys) {
	switch (key.type) {
	    using enum DataType;
	case Unsigned8:
	    {
		auto a = *reinterpret_cast<const uint8_t*>(a_ptr + key.offset);
		auto b = *reinterpret_cast<const uint8_t*>(b_ptr + key.offset);
		if (a < b) return true;
		else if (a > b) return false;
	    }
	    break;
	    
	case Unsigned16:
	    {
		auto a = *reinterpret_cast<const uint16_t*>(a_ptr + key.offset);
		auto b = *reinterpret_cast<const uint16_t*>(b_ptr + key.offset);
		if (a < b) return true;
		else if (a > b) return false;
	    }
	    break;
	    
	case Unsigned32:
	    {
		auto a = *reinterpret_cast<const uint32_t*>(a_ptr + key.offset);
		auto b = *reinterpret_cast<const uint32_t*>(b_ptr + key.offset);
		if (a < b) return true;
		else if (a > b) return false;
	    }
	    break;
	    
	case Unsigned64:
	    {
		auto a = *reinterpret_cast<const uint64_t*>(a_ptr + key.offset);
		auto b = *reinterpret_cast<const uint64_t*>(b_ptr + key.offset);
		if (a < b) return true;
		else if (a > b) return false;
	    }
	    break;
	    
	case Signed64:
	    {
		auto a = *reinterpret_cast<const int64_t*>(a_ptr + key.offset);
		auto b = *reinterpret_cast<const int64_t*>(b_ptr + key.offset);
		if (a < b) return true;
		else if (a > b) return false;
	    }
	    break;
	}
    }
    return false;
}
