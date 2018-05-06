#pragma once

#include "../util/store/ArenaString.h"
#include "./Path.h"

struct Path::Impl {
	Option<Path> parent;
	ArenaString name;

	hash_t str_len() const {
		return (parent.has() ? parent.get().impl->str_len() : 0) + name.slice().size();
	}

	inline friend bool operator==(const Path::Impl& a, const Path::Impl& b) {
		return a.parent == b.parent && a.name == b.name;
	}
	inline friend bool operator!=(const Path::Impl& a, const Path::Impl& b) {
		return !(a == b);
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
		hash_t operator()(const Path::Impl& p) const {
			//TODO:PERF
			return StringSlice::hash{}(p.name);
		}
	};
};
