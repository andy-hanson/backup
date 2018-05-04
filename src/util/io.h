#pragma once

#include "./Grow.h"
#include "./Option.h"
#include "./Alloc.h"
#include "../host/Path.h"

struct FileLocator {
	const StringSlice& root;
	const Path& path;
	const StringSlice& extension;

	// Mutates 'out' to point to the remainder.
	friend MutableStringSlice& operator<<(MutableStringSlice& out, const FileLocator& loc) {
		loc.path.write(loc.root, loc.extension, out);
		return out;
	}
	template <uint size>
	inline const char* get_cstring(MaxSizeString<size>& m) const {
		MutableStringSlice s = m.slice();
		s << *this;
		s << '\0';
		return m.slice().begin;
	}
};

Option<ArenaString> try_read_file(const FileLocator& loc, Arena& out, bool null_terminated);
void write_file(const FileLocator& loc, const Grow<char>& contents);
void delete_file(const FileLocator& loc);
bool file_exists(const FileLocator& loc);
