#pragma once

#include "../../util/store/Arena.h"
#include "../../util/store/Slice.h"
#include "../../util/store/StringSlice.h"

struct TypeAst;

// TypeAst may be:
// * ?T
// * Vec<Int>
// May also specify lifetime variables:
// `get-x Int *p(p Point)`
// `get-x-of-one-of-them Int *p1 *p2(p1 Point, p2 Point)
// `push-point-pointer Void(v Vec<Point *p>, p Point)
// `mix-vectors Void(v1 Vec<Point *?l>, v2 Vec<Point *?l>) ?l`
class StoredTypeAst {
public:
	enum Kind { TypeParameter, InstStruct };

private:
	Kind _kind;
	StringSlice _name;
	Slice<TypeAst> _type_arguments; // only for InstStruct

public:
	StoredTypeAst(StringSlice name) : _kind(Kind::TypeParameter), _name(name) {}
	StoredTypeAst(StringSlice name, Slice<TypeAst> type_arguments) : _kind(Kind::InstStruct), _name(name), _type_arguments(type_arguments) {}

	Kind kind() const { return _kind; }
	const StringSlice& name() const { return _name; }
	const Slice<TypeAst>& type_arguments() const { assert(_kind == Kind::InstStruct); return _type_arguments; }
};

struct LifetimeConstraintAst {
	// ParameterName should be a compile error in a struct declaration.
	enum class Kind { ParameterName, LifetimeVariableName };
	Kind kind;
	StringSlice name;
};

struct TypeAst {
	StoredTypeAst stored;
	// User can write '*foo' after a time to limit its lifetime.
	// In return or struct field, that also changes it from by-value/new to being a pointer.
	// Parameters are always pointers without explicit 'new'.
	Slice<LifetimeConstraintAst> lifetime_constraints;
};
