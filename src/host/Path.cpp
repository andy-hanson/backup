#include "./Path.h"

#include "../util/Map.h"
#include "../util/Set.h"

struct Path::Impl {
	Option<Path> parent;
	ArenaString name;

	size_t str_len() const {
		return (parent.has() ? parent.get().impl->str_len() : 0) + name.slice().size();
	}

	inline friend bool operator==(const Path::Impl& a, const Path::Impl& b) {
		return a.parent == b.parent && a.name == b.name;
	}

	template <typename WriterLike>
	static void to_string_worker(WriterLike& b, const Path::Impl& p) {
		if (p.parent.has()) {
			Path::Impl::to_string_worker(b, p.parent.get().impl);
			b << '/';
		}
		b << p.name;
	}

	struct hash {
		size_t operator()(const Path::Impl& p) const {
			//TODO:PERF
			return StringSlice::hash{}(p.name);
		}
	};
};

namespace {
	template <typename /*StringSlice => void*/ Cb>
	void split_string(const StringSlice& s, Cb cb) {
		const char* c = s.begin();
		const char* part_begin = c;
		while (true) {
			if (*c == '\0' || *c == '/') {
				cb(StringSlice { part_begin, c });
				if (*c == '\0') return;
				part_begin = c + 1;
			}
			++c;
		}
	}

	ArenaString get_name(Map<StringSlice, ArenaString, StringSlice::hash>& m, Arena& arena, const StringSlice& name) {
		Option<const ArenaString&> already = m.get(name);
		if (already.has()) {
			return already.get();
		} else {
			ArenaString a = arena.str(name);
			m.must_insert(a, a);
			return m.get(name).get(); //TODO:PERF
		}
	}
}

const Option<Path>& Path::parent() const {
	return impl->parent;
}
const ArenaString& Path::base_name() const {
	return impl->name;
}

Writer& operator<<(Writer& out, const Path& path) {
	Path::Impl::to_string_worker(out, path.impl);
	return out;
}

ArenaString Path::to_cstring(const StringSlice& root, Arena& out, const StringSlice& extension) const {
	Arena::StringBuilder b = out.string_builder(root.size() + impl->str_len() + extension.size() + 3); // + 1 for slash, + 1 for '.', + 1 for \0
	b << root;
	b << '/';
	Path::Impl::to_string_worker(b, impl);
	b << '.';
	b << extension;
	b << '\0';
	return b.finish();
}

struct PathCache::Impl {
	Set<Path::Impl, Path::Impl::hash> paths;
	Map<StringSlice, ArenaString, StringSlice::hash> slices;
	Arena arena;
};

PathCache::PathCache() : impl(unique_ptr<PathCache::Impl> { new PathCache::Impl() }) {}
PathCache::~PathCache() {}; // impl implicitly deleted

Path PathCache::from_path_string(const StringSlice& s) {
	Option<Path> p;
	split_string(s, [&](const StringSlice& name) {
		p = resolve(p, name);
	});
	return p.get();
}
Path PathCache::from_part_slice(const StringSlice& s) {
	for (char c : s)
		assert(c != '/');
	return resolve(Option<Path>{}, s);
}

Path PathCache::resolve(Path parent, const StringSlice& child) {
	return resolve(Option { parent }, child);
}
Path PathCache::resolve(Option<Path> parent, const StringSlice& child) {
	return Path { impl->paths.get_in_set_or_insert(Path::Impl { parent, get_name(impl->slices, impl->arena, child) }) };
}

Option<Path> PathCache::resolve(Path resolve_from, const RelPath& rel) {
	// Climb up n_parens in resolve_from.
	for (uint n_parents = rel.n_parents; n_parents != 0; --n_parents) {
		const Option<Path>& parent = resolve_from.parent();
		if (!parent.has())
			return n_parents == 1 ? Option { rel.path } : Option<Path>{};
		resolve_from = parent.get();
	}

	// Now add things onto the end.
	MaxSizeVector<16, StringSlice> parents;
	Option<Path> p = rel.path.parent();
	for (; p.has(); p = p.get().parent())
		parents.push(p.get().base_name());

	while (!parents.empty())
		p = resolve(p, parents.pop_and_return());
	return Option { resolve(p, rel.path.base_name()) };
}