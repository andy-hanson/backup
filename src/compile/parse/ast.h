#pragma once

#include "../../util/store/ArenaString.h"
#include "../../util/store/Slice.h"
#include "../../util/store/StringSlice.h"
#include "../../util/store/List.h"
#include "../../util/Path.h"
#include "../model/effect.h"
#include "./expr_ast.h"

struct TypeParameterAst {
	StringSlice name;
	uint index;
};

struct StructFieldAst {
	Option<ArenaString> comment;
	StringSlice name;
	TypeAst type;
};

class StructBodyAst {
public:
	enum class Kind { CppName, Fields };
private:
	union Data {
		StringSlice cpp_name;
		Slice<StructFieldAst> fields;
		Data() {}
	};
	Kind _kind;
	Data _data;

public:
	inline explicit StructBodyAst(StringSlice cpp_name) : _kind(Kind::CppName) {
		_data.cpp_name = cpp_name;
	}
	inline explicit StructBodyAst(Slice<StructFieldAst> fields) : _kind(Kind::Fields) {
		_data.fields = fields;
	}

	Kind kind() const { return _kind; }
	const StringSlice& cpp_name() const { assert(_kind == Kind::CppName); return _data.cpp_name; }
	const Slice<StructFieldAst>& fields() const { assert(_kind == Kind::Fields); return _data.fields; }
};


struct ParameterAst {
	StringSlice name;
	TypeAst type;
};

struct SpecUseAst {
	StringSlice spec;
	Slice<TypeAst> type_arguments;
};

struct FunSignatureAst {
	Option<ArenaString> comment;
	StringSlice name;
	Option<Effect> effect;
	TypeAst return_type;
	Slice<ParameterAst> parameters;
	Slice<TypeParameterAst> type_parameters;
	Slice<SpecUseAst> spec_uses;
};

struct StructDeclarationAst {
	Option<ArenaString> comment;
	SourceRange range;
	bool is_public;
	StringSlice name;
	Slice<TypeParameterAst> type_parameters;
	bool copy;
	StructBodyAst body;
};

struct SpecDeclarationAst {
	Option<ArenaString> comment;
	SourceRange range;
	bool is_public;
	StringSlice name;
	Slice<TypeParameterAst> type_parameters;
	Slice<FunSignatureAst> signatures;
};

class FunBodyAst {
public:
	enum class Kind { CppSource, Expression };

private:
	union Data {
		ExprAst expression; // Ref so we don't have to depend on the definition of Expression here.
		ArenaString cpp_source;
		Data() {} // uninitialized
		~Data() {} // string freed by ~AnyBody
	};
	Kind _kind;
	Data _data;

public:
	inline FunBodyAst(const FunBodyAst& other) { *this = other; }
	void operator=(const FunBodyAst& other);
	inline FunBodyAst(ArenaString cpp_source) : _kind(Kind::CppSource) {
		_data.cpp_source = cpp_source;
	}
	inline FunBodyAst(ExprAst expression) : _kind(Kind::Expression) {
		_data.expression = expression;
	}

	inline Kind kind() const { return _kind; }
	inline const ArenaString& cpp_source() const { assert(_kind == Kind::CppSource); return _data.cpp_source; }
	inline const ExprAst& expression() const { assert(_kind == Kind::Expression); return _data.expression; }
};

struct FunDeclarationAst {
	bool is_public;
	FunSignatureAst signature;
	FunBodyAst body;
};

struct ImportAst {
	SourceRange range;
	Option<uint> n_parents; // None for global import
	Path path;
};

struct FileAst {
	Path path;
	StringSlice source;
	Option<ArenaString> comment;
	Slice<ImportAst> imports;
	List<StringSlice> includes;
	List<SpecDeclarationAst> specs;
	List<StructDeclarationAst> structs;
	List<FunDeclarationAst> funs;

	FileAst(Path _path, StringSlice _source) : path(_path), source(_source) {}
};
