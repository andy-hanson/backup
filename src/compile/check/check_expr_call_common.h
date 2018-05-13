#pragma once

#include "../diag/diag.h"
#include "../model/BuiltinTypes.h"
#include "../model/model.h"
#include "../model/expr.h" // Let
#include "../../util/store/Arena.h"
#include "../../util/store/ListBuilder.h"

#include "./CheckCtx.h"

struct ExpressionAndLifetime {
	Expression expression;
	Lifetime lifetime;
};

struct ExprContext {
	CheckCtx& check_ctx;
	Arena& scratch_arena; // cleared after every convert call.
	const FunsTable& funs_table;
	const StructsTable& structs_table;
	Ref<const FunDeclaration> current_fun;
	// This is pushed and popped as we add locals and go out of scope.
	MaxSizeVector<8, Ref<const Let>> locals;
	const BuiltinTypes& builtin_types;

	ExprContext(const ExprContext& other) = delete;
	void operator=(const ExprContext& other) = delete;
};

// In the type checker we don't consider lifetime information. So only keep a StoredType, not Type.
// So if the StoredType is an InstStrcut, its type parameters will not have lifetimes.
class Expected {
	const bool _had_expectation;
	bool _was_checked = false; // For asserting
	Option<StoredType> type;

public:
	inline Expected() : _had_expectation{false} {}
	inline Expected(StoredType t) : _had_expectation{true}, type{t} {}

	inline ~Expected() {
		assert(_was_checked);
	}

	// Either an initial expectation, or one inferred from something previous.
	inline const Option<StoredType>& get_current_expectation() const {
		return type;
	}

	//TODO: when to use this vs has_inferred_type?
	inline bool had_expectation() const { return _had_expectation; }

	inline bool has_expectation_or_inferred_type() const {
		return type.has();
	}

	inline const StoredType& inferred_stored_type() const {
		return type.get();
	}

	// Called when an operation behaves like checking this, even without calling 'check'.
	// E.g., removing overloads that don't return the expected type.
	inline void as_if_checked() {
		_was_checked = true;
	}

	ExpressionAndLifetime bogus() {
		check_no_infer(StoredType::bogus());
		return { Expression::bogus(), Lifetime::noborrow() };
	}

	void check_no_infer(StoredType actual);

	void set_inferred(StoredType actual);
};
