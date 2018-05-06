#pragma once

#include "../../util/store/ListBuilder.h"
#include "../../util/Path.h"
#include "../diag/diag.h"
#include "../model/model.h"

struct CheckCtx {
	Arena& arena;
	const StringSlice& source;
	Path path; // Path of current module
	const Slice<Ref<const Module>>& imports;
	ListBuilder<Diagnostic>& diags;

	inline SourceRange range(StringSlice slice) {
		return SourceRange::inner_slice(source, slice);
	}
	inline void diag(SourceRange range, Diag diag) {
		diags.add({ path, range, diag }, arena);
	}
	inline void diag(StringSlice slice, Diag d) {
		diag(range(slice), d);
	}
	inline Option<ArenaString> copy_str(Option<ArenaString> s) {
		return s.has() ? Option { str(arena, s.get()) } : Option<ArenaString> {};
	}
};

inline Identifier id(CheckCtx& al, StringSlice s) {
	return Identifier { str(al.arena, s) };
}
