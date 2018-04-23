#pragma once

#include "../../util/StringSlice.h"
#include "../model/effect.h"
#include "./expr_ast.h"

struct TypeParameterAst {
	StringSlice name;
	uint index;
};

struct StructFieldAst {
	TypeAst type;
	StringSlice name;
};

class StructBodyAst {
public:
	enum class Kind { CppName, Fields };
private:
	union Data {
		StringSlice cpp_name;
		Arr<StructFieldAst> fields;
		Data() {}
	};
	Kind _kind;
	Data _data;

public:
	StructBodyAst(StringSlice cpp_name) : _kind(Kind::CppName) {
		_data.cpp_name = cpp_name;
	}
	StructBodyAst(Arr<StructFieldAst> fields) : _kind(Kind::Fields) {
		_data.fields = fields;
	}

	Kind kind() const { return _kind; }
	const StringSlice& cpp_name() const { assert(_kind == Kind::CppName); return _data.cpp_name; }
	const Arr<StructFieldAst>& fields() const { assert(_kind == Kind::Fields); return _data.fields; }
};


struct ParameterAst {
	bool from;
	Option<Effect> effect;
	TypeAst type;
	StringSlice name;
};

struct SpecUseAst {
	StringSlice spec;
	Arr<TypeAst> type_arguments;
};

struct FunSignatureAst {
	StringSlice name;
	Option<Effect> effect;
	TypeAst return_type;
	Arr<ParameterAst> parameters;
	Arr<TypeParameterAst> type_parameters;
	Arr<SpecUseAst> spec_uses;
};

struct StructDeclarationAst {
	SourceRange range;
	StringSlice name;
	Arr<TypeParameterAst> type_parameters;
	bool copy;
	StructBodyAst body;
};

struct SpecDeclarationAst {
	StringSlice name;
	Arr<TypeParameterAst> type_parameters;
	Arr<FunSignatureAst> signatures;
};

class FunBodyAst {
public:
	enum class Kind { CppSource, Expression };

private:
	union Data {
		ExprAst expression; // Ref so we don't have to depend on the definition of Expression here.
		StringSlice cpp_source;
		Data() {} // uninitialized
		~Data() {} // string freed by ~AnyBody
	};
	Kind _kind;
	Data _data;

public:
	FunBodyAst(const FunBodyAst& other) {
		*this = other;
	}
	void operator=(const FunBodyAst& other) {
		_kind = other._kind;
		switch (_kind) {
			case Kind::CppSource:
				_data.cpp_source = other._data.cpp_source;
				break;
			case Kind::Expression:
				_data.expression = other._data.expression;
				break;
		}
	}
	FunBodyAst(StringSlice cpp_source) : _kind(Kind::CppSource) {
		_data.cpp_source = cpp_source;
	}
	FunBodyAst(ExprAst expression) : _kind(Kind::Expression) {
		_data.expression = expression;
	}

	Kind kind() const { return _kind; }
	const StringSlice& cpp_source() const { assert(_kind == Kind::CppSource); return _data.cpp_source; }
	const ExprAst& expression() const { assert(_kind == Kind::Expression); return _data.expression; }
};

struct FunDeclarationAst {
	FunSignatureAst signature;
	FunBodyAst body;
};

class DeclarationAst {
public:
	enum class Kind { CppInclude, Struct, Spec, Fun };
private:
	union Data {
		StringSlice cpp_include;
		StructDeclarationAst strukt;
		SpecDeclarationAst spec;
		FunDeclarationAst fun;
		Data() {}
		~Data() {}
	};
	Kind _kind;
	Data _data;

public:
	DeclarationAst(const DeclarationAst& other) {
		*this = other;
	}
	void operator=(const DeclarationAst& other) {
		_kind = other._kind;
		switch (_kind) {
			case Kind::CppInclude:
				_data.cpp_include = other._data.cpp_include;
				break;
			case Kind::Struct:
				_data.strukt = other._data.strukt;
				break;
			case Kind::Spec:
				_data.spec = other._data.spec;
				break;
			case Kind::Fun:
				_data.fun = other._data.fun;
				break;
		}
	}

	DeclarationAst(StringSlice cpp_include) : _kind(Kind::CppInclude) { _data.cpp_include = cpp_include; }
	DeclarationAst(StructDeclarationAst strukt) : _kind(Kind::Struct) { _data.strukt = strukt; }
	DeclarationAst(SpecDeclarationAst spec) : _kind(Kind::Spec) { _data.spec = spec; }
	DeclarationAst(FunDeclarationAst fun) : _kind(Kind::Fun) { _data.fun = fun; }

	Kind kind() const { return _kind; }
	const StructDeclarationAst& strukt() const { assert(_kind == Kind::Struct); return _data.strukt; }
	const SpecDeclarationAst& spec() const { assert(_kind == Kind::Spec); return _data.spec; }
	const FunDeclarationAst& fun() const { assert(_kind == Kind::Fun); return _data.fun; }
};


struct File {

};


