#pragma once

#include "../../util/Alloc.h"
#include "../../util/StringSlice.h"

struct Identifier {
	ArenaString str;
	bool operator==(const Identifier& i) const { return str.slice() == i.str.slice(); }
	bool operator==(const StringSlice& s) const { return str == s; }
};
namespace std {
	template<>
	struct hash<Identifier> {
		size_t operator()(Identifier i) const { return hash<StringSlice>{}(i.str.slice()); }
	};
}


