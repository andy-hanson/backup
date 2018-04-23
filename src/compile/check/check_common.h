#pragma once

#include "../model/model.h"
#include "../../util/Alloc.h"
#include "./type_utils.h"
#include "./BuiltinTypes.h"

struct Al {
	Arena& arena;
	StringSlice source;
	Vec<Diagnostic>& diags;

	inline SourceRange range(StringSlice slice) {
		return source.range_from_inner_slice(slice);
	}
	inline void diag(SourceRange range, Diag diag) {
		diags.push({ range, diag });
	}
	inline void diag(StringSlice slice, Diag d) {
		diag(range(slice), d);
	}
};

inline Identifier id(Al& al, StringSlice s) {
	return Identifier { al.arena.str(s) };
}

// current_specs: the specs from the current function.
inline void check_param_or_local_shadows_fun(Al& al, const StringSlice& name, const FunsTable& funs_table, const Arr<SpecUse>& current_specs) {
	if (funs_table.has(name))
		al.diag(name, Diag::Kind::LocalShadowsFun);
	for (const SpecUse& spec_use : current_specs)
		for (const FunSignature& sig : spec_use.spec->signatures)
			if (sig.name == name)
				al.diag(name,Diag::Kind::LocalShadowsSpecSig);
}

struct ExprContext {
	Al& al;
	Arena& scratch_arena; // cleared after every convert call.
	const FunsTable& funs_table;
	const StructsTable& structs_table;
	ref<const FunDeclaration> current_fun;
	// This is pushed and popped as we add locals and go out of scope.
	MaxSizeVector<8, ref<const Let>> locals;
	const BuiltinTypes& builtin_types;

	ExprContext(const ExprContext& other) = delete;
	void operator=(const ExprContext& other) = delete;
};

class Expected {
	const bool _had_expectation;
	bool _was_checked = false; // For asserting
	Option<Type> type;

public:
	Expected() : _had_expectation(false) {}
	__attribute__((unused)) // https://youtrack.jetbrains.com/issue/CPP-12376
	Expected(Type t) : _had_expectation(true), type(t) {}

	~Expected() {
		assert(_was_checked);
	}

	// Either an initial expectation, or one inferred from something previous.
	const Option<Type>& get_current_expectation() const {
		return type;
	}

	//TODO: when to use this vs has_inferred_type?
	bool had_expectation() const { return _had_expectation; }

	bool has_expectation_or_inferred_type() const {
		return type;
	}

	const Type& inferred_type() const {
		return type.get();
	}

	// Called when an operation behaves like checking this, even without calling 'check'.
	// E.g., removing overloads that don't return the expected type.
	void as_if_checked() {
		_was_checked = true;
	}

	void check_no_infer(Type actual) {
		_was_checked = true;
		if (type) {
			Type t = type.get();
			if (!does_type_match_no_infer(t, actual)) {
				throw "todo";
			}
		}
		else {
			assert(!_had_expectation);
			set_inferred(actual);
		}
	}

	void set_inferred(Type actual) {
		assert(!_had_expectation && !type);
		_was_checked = true;
		type = actual;
	}
};
