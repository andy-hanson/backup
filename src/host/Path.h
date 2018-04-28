#pragma once

#include <string>
#include "../util/Alloc.h"
#include "../util/Option.h"
#include "../util/Map.h"
#include "../util/StringSlice.h"

class Path {
	friend class PathCache;
	Option<ref<const Path>> _parent;
	ArenaString _name;

	size_t str_len() const {
		return (_parent.has() ? _parent.get()->str_len() : 0) + _name.slice().size();
	}

	void to_string_worker(Arena::StringBuilder& b) const {
		if (_parent.has())
			_parent.get()->to_string_worker(b);
		b << _name;
	}

	Path(Option<ref<const Path>> parent, ArenaString name) : _parent(parent), _name(name) {}

public:
	void to_string(std::string& s) const {
		if (_parent.has()) {
			_parent.get()->to_string(s);
			s += '/';
		}
		for (char c : _name.slice())
			s += c;
	}

	ArenaString to_cstring(const std::string& root, Arena& out, const StringSlice& extension) const {
		Arena::StringBuilder b = out.string_builder(root.size() + str_len() + extension.size() + 3); // + 1 for slash, + 1 for '.', + 1 for \0
		b << root;
		b << '/';
		to_string_worker(b);
		b << '.';
		b << extension;
		b << '\0';
		return b.finish();
	}

	const Option<ref<const Path>>& parent() const {
		return _parent;
	}

	const ArenaString& base_name() const {
		return _name;
	}

	friend bool operator==(const Path& a, const Path& b);
};
namespace std {
	template <>
	struct hash<Path> {
		size_t operator()(const Path& p) const {
			//TODO:PERF
			return std::hash<StringSlice>{}(p.base_name());
		}
	};
}

// Path is like 'a/b/c'. RelPath is like '../../a/b/c'.
struct RelPath {
	uint n_parents;
	ref<const Path> path;
};

//We want to construct a given path only once.
//So for every (parent, name) combo, we store an allocation of it only once.
class PathCache {
	Set<Path> paths;
	Map<StringSlice, ArenaString> slices;
	Arena arena;

public:
	ref<const Path> from_string(const std::string& s);
	// Returns a Path with a single part.
	ref<const Path> from_part_slice(const StringSlice& s);
	ref<const Path> resolve(ref<const Path> parent, const StringSlice& child);
	ref<const Path> resolve(Option<ref<const Path>> parent, const StringSlice& child);
	// Returns None if the relative path is invalid.
	Option<ref<const Path>> resolve(ref<const Path> p, const RelPath& r);
};

