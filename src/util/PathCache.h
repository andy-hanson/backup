#pragma once

#include "./unique_ptr.h"
#include "./Path.h"

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
