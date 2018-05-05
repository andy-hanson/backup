#pragma once

#include "./Arena.h"
#include "./BlockedList.h"
#include "./Option.h"
#include "../util/Path.h"

struct FileLocator {
	StringSlice root;
	Path path;
	StringSlice extension;

	inline FileLocator with_extension(const StringSlice& new_extension) const {
		return { root, path, new_extension };
	}

	// Mutates 'out' to point to the remainder.
	friend MutableStringSlice& operator<<(MutableStringSlice& out, const FileLocator& loc) {
		loc.path.write(out, loc.root, Option<const StringSlice&> { loc.extension });
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

class DirectoryEntry {
	friend class DirectoryIterator;
	MaxSizeString<64> _name;
	uint str_len;
	bool _is_directory;

public:
	inline StringSlice name() {
		StringSlice s = _name.slice();
		return s.slice(str_len);
	}
	inline bool is_directory() const { return _is_directory; }
};

/*abstract*/ class DirectoryIteratee {
public:
	virtual void on_file(const StringSlice& name) = 0;
	virtual void on_directory(const StringSlice& name) = 0;
	// https://stackoverflow.com/a/29217604
	// If we make this '= 0' there is a compiler warning.
	virtual ~DirectoryIteratee();
};

void list_directory(const StringSlice& loc, DirectoryIteratee& iteratee);

Option<StringSlice> try_read_file(const FileLocator& loc, Arena& out, bool null_terminated);
void write_file(const FileLocator& loc, const BlockedList<char>& contents);
void delete_file(const FileLocator& loc);
bool file_exists(const FileLocator& loc);
