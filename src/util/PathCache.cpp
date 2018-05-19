#include "./PathCache.h"

#include "../util/store/Map.h"
#include "../util/store/Set.h"
#include "./PathImpl.h"

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

	using Slices = Map<StringSlice, ArenaString, StringSlice::hash>;

	ArenaString get_name(Slices& slices, Arena& arena, const StringSlice& name) {
		Option<ArenaString&> already = slices.get(name);
		if (already.has()) {
			return already.get();
		} else {
			ArenaString a = copy_string(arena, name);
			slices.must_insert(a, a);
			return slices.get(name).get(); //TODO:PERF
		}
	}
}

struct PathCache::Impl {
	Arena arena;
	Set<Path::Impl, Path::Impl::hash> paths;
	Slices slices;

	Impl() : arena{}, paths{32, arena}, slices{32, arena} {}
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
	return Path { &impl->paths.get_in_set_or_insert(Path::Impl { parent, get_name(impl->slices, impl->arena, child) }) };
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

	while (!parents.is_empty())
		p = resolve(p, parents.pop_and_return());
	return Option { resolve(p, rel.path.base_name()) };
}
