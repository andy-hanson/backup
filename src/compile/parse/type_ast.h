#pragma once

#include "../../util/Arena.h"
#include "../../util/StringSlice.h"

class TypeAst {
public:
	enum Kind { Parameter, InstStruct };

private:
	Kind _kind;
	StringSlice _name;
	Arr<TypeAst> _type_arguments; // only for InstStruct

public:
	TypeAst(StringSlice name) : _kind(Kind::Parameter), _name(name) {}
	TypeAst(StringSlice name, Arr<TypeAst> type_arguments) : _kind(Kind::InstStruct), _name(name), _type_arguments(type_arguments) {}

	Kind kind() const { return _kind; }
	const StringSlice& name() const { return _name; }
	const Arr<TypeAst>& type_arguments() const { assert(_kind == Kind::InstStruct); return _type_arguments; }
};
