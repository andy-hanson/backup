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
	friend MaxSizeStringWriter& operator<<(MaxSizeStringWriter& out, const FileLocator& loc) {
		loc.path.write(out, loc.root, Option<const StringSlice&> { loc.extension });
		return out;
	}

	template <uint size>
	inline const char* get_cstring(MaxSizeStringStorage<size>& builder) const {
		MaxSizeStringWriter s = builder.writer();
		s << *this << '\0';
		return builder.finish(s).begin();
	}
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
