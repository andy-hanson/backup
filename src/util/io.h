#pragma once

#include "./store/Arena.h"
#include "store/MaxSizeString.h"
#include "./Option.h"
#include "./Path.h"
#include "./Writer.h"

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
void write_file(const FileLocator& loc, const Writer::Output& contents);
void delete_file(const FileLocator& loc);
bool file_exists(const FileLocator& loc);
