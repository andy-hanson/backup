#include "./Path.h"

namespace {
	template <typename /*StringSlice => void*/ Cb>
	void split_string(const std::string& s, Cb cb) {
		const char* c = s.begin().base();
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

	ArenaString get_name(Map<StringSlice, ArenaString>& m, Arena& arena, const StringSlice& name) {
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

bool operator==(const Path& a, const Path& b) {
	return a._parent == b._parent && a._name == b._name;
}

ref<const Path> PathCache::from_string(const std::string& s) {
	Option<ref<const Path>> p;
	split_string(s, [&](const StringSlice& name) {
		p = resolve(p, name);
	});
	return p.get();
}
ref<const Path> PathCache::from_part_slice(const StringSlice& s) {
	for (char c : s)
		assert(c != '/');
	return resolve({}, s);
}

ref<const Path> PathCache::resolve(ref<const Path> parent, const StringSlice& child) {
	return resolve(Option<ref<const Path>>{parent}, child);
}
ref<const Path> PathCache::resolve(Option<ref<const Path>> parent, const StringSlice& child) {
	return paths.get_in_set_or_insert(Path { parent, get_name(slices, arena, child) });
}

Option<ref<const Path>> PathCache::resolve(ref<const Path> resolve_from, const RelPath& rel) {
	// Climb up n_parens in resolve_from.
	for (uint n_parents = rel.n_parents; n_parents != 0; --n_parents) {
		const Option<ref<const Path>>& parent = resolve_from->parent();
		if (!parent.has())
			return n_parents == 1 ? Option<ref<const Path>>{rel.path} : Option<ref<const Path>>{};
		resolve_from = parent.get();
	}

	// Now add things onto the end.
	MaxSizeVector<16, StringSlice> parents;
	Option<ref<const Path>> p = rel.path->parent();
	for (; p.has(); p = p.get()->parent())
		parents.push(p.get()->base_name());

	while (!parents.empty())
		p = resolve(p, parents.pop_and_return());
	return Option<ref<const Path>> { resolve(p, rel.path->base_name()) };
}
