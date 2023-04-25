// Copyright (C) 2022, 2023 by Mark Melton
//

#pragma once
#include "core/string/lexical_cast.h"

enum class DataType { Signed64, Unsigned8, Unsigned16, Unsigned32, Unsigned64 };

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

std::ostream& operator<<(std::ostream& os, DataType type) {
    switch (type) {
	using enum DataType;
    case Signed64:
	os << "i64";
	break;
    case Unsigned8:
	os << "u8";
	break;
    case Unsigned16:
	os << "u16";
	break;
    case Unsigned32:
	os << "u32";
	break;
    case Unsigned64:
	os << "u64";
	break;
    }
    return os;
}

