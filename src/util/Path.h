#pragma once

#include "./store/Arena.h"
#include "./store/MaxSizeStringStorage.h"
#include "./store/StringSlice.h"
#include "./Option.h"
#include "./unique_ptr.h"
#include "./Writer.h"

// Can only be constructed through a PathCache.
class Path {
	struct Impl;
	friend class PathCache;
	Ref<const Impl> impl;

	Path(Ref<const Impl> _impl) : impl(_impl) {}

public:
	friend Writer& operator<<(Writer& out, const Path& path);
	void write(MaxSizeStringWriter& out, const StringSlice& root, Option<const StringSlice&> extension) const;

	const Option<Path>& parent() const;
	StringSlice base_name() const;

	inline friend bool operator==(const Path& a, const Path& b) {
		// Paths are memoized, so we can just compare references.
		return a.impl == b.impl;
	}

	struct hash {
		hash_t operator()(const Path& p) const;
	};
};
inline bool operator!=(const Path& a, const Path& b) { return !(a == b); }

// Path is like 'a/b/c'. RelPath is like '../../a/b/c'.
struct RelPath {
	uint n_parents;
	Path path;
};

// Memoizes construction of paths.
class PathCache {
	struct Impl;
	unique_ptr<Impl> impl;
	PathCache(const PathCache& other) = delete;

public:
	PathCache();
	~PathCache();
	// Parses a Path with multiple parts separated by '/'
	Path from_path_string(const StringSlice& s);
	// Returns a Path with a single part.
	Path from_part_slice(const StringSlice& s);
	Path resolve(Path parent, const StringSlice& child);
	Path resolve(Option<Path> parent, const StringSlice& child);
	// Returns None if the relative path is invalid.
	Option<Path> resolve(Path p, const RelPath& r);
};
