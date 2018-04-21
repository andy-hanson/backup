#pragma once

#include <cassert>

#include "../../util/Alloc.h"
#include "../../util/hash_util.h"
#include "../../util/Map.h"
#include "../../util/Vec.h"
#include "../diag/diag.h"
#include "./Identifier.h"

#include "./effect.h"

class Expression;

struct Module;

struct TypeParameter {
	SourceRange range;
	Identifier name;
	uint index;
};

struct StructField;

class StructBody {
public:
	enum class Kind { Nil, Fields, CppName };
private:
	union Data {
		DynArray<StructField> fields;
		ArenaString cpp_name;

		Data() {} //uninitialized
		~Data() {}
	};
	Kind _kind;
	Data data;

public:
	StructBody() : _kind(Kind::Nil) {}
	StructBody(const StructBody& other) { *this = other; }
	void operator=(const StructBody& other) {
		_kind = other._kind;
		switch (_kind) {
			case Kind::Nil: break;
			case Kind::Fields:
				data.fields = other.data.fields;
				break;
			case Kind::CppName:
				data.cpp_name = other.data.cpp_name;
				break;
		}
	}
	StructBody(DynArray<StructField> fields) : _kind(Kind::Fields) {
		data.fields = fields;
	}
	StructBody(ArenaString cpp_name) : _kind(Kind::CppName) {
		data.cpp_name = cpp_name;
	}

	Kind kind() const { return _kind; }
	bool is_fields() const { return _kind == StructBody::Kind::Fields; }
	DynArray<StructField>& fields() {
		assert(_kind == Kind::Fields);
		return data.fields;
	}
	const DynArray<StructField>& fields() const {
		assert(_kind == Kind::Fields);
		return data.fields;
	}
	const ArenaString& cpp_name() const {
		assert(_kind == Kind::CppName);
		return data.cpp_name;
	}
};

struct StructDeclaration {
	ref<const Module> containing_module;
	SourceRange range;
	DynArray<TypeParameter> type_parameters;
	Identifier name;
	StructBody body;

	StructDeclaration(ref<const Module> _containing_module, SourceRange _range, DynArray<TypeParameter> _type_parameters, Identifier _name)
		: containing_module(_containing_module), range(_range), type_parameters(_type_parameters), name(_name) {}

	size_t arity() const { return type_parameters.size(); }
};

class Type;

struct InstStruct {
	ref<const StructDeclaration> strukt;
	DynArray<Type> type_arguments;
};
namespace std {
	template <>
	struct hash<InstStruct> {
		size_t operator()(const InstStruct& i) const {
			return hash_combine(hash<::ref<const StructDeclaration>>{}(i.strukt), hash_dyn_array(i.type_arguments));
		}
	};
}
bool operator==(const InstStruct& a, const InstStruct& b);

struct PlainType {
	Effect effect;
	InstStruct inst_struct;

	bool is_deeply_plain() const;
};
namespace std {
	template <>
	struct hash<PlainType> {
		size_t operator()(const PlainType& p) const {
			return hash_combine(size_t(p.effect), p.inst_struct);
		}
	};
}
bool operator==(const PlainType& a, const PlainType& b);

class Type {
public:
	enum Kind { Nil, Plain, Param };
private:
	union Data {
		PlainType plain;
		ref<const TypeParameter> param;
		Data() {} // uninitialized
		~Data() {}
	};

	Kind _kind;
	Data data;

public:
	Kind kind() const { return _kind; }

	Type() : _kind(Kind::Nil) {}
	Type(const Type& other) {
		*this = other;
	}
	void operator=(const Type& other) {
		_kind = other._kind;
		switch (other._kind) {
			case Kind::Nil: break;
			case Kind::Plain: data.plain = other.data.plain; break;
			case Kind::Param: data.param = other.data.param; break;
		}
	}
	explicit Type(PlainType p) : _kind(Kind::Plain) {
		data.plain = p;
	}

	explicit Type(ref<const TypeParameter> param) : _kind(Kind::Param) {
		data.param = param;
	}

	bool is_plain() const {
		return _kind == Kind::Plain;
	}
	bool is_parameter() const {
		return _kind == Kind::Param;
	}

	const PlainType& plain() const {
		assert(_kind == Kind::Plain);
		return data.plain;
	}

	ref<const TypeParameter> param() const {
		assert(_kind == Kind::Param);
		return data.param;
	}
};
namespace std {
	template <>
	struct hash<Type> {
		size_t operator()(const Type& t) const {
			switch (t.kind()) {
				case Type::Kind::Nil:
				case Type::Kind::Param:
					throw "todo";
				case Type::Kind::Plain:
					return hash<PlainType>{}(t.plain());
			}
		}
	};
}
bool operator==(const Type& a, const Type& b);
inline bool operator!=(const Type& a, const Type& b) { return !(a == b); }

struct StructField {
	Type type;
	Identifier name;
};

struct Parameter {
	Type type;
	Identifier name;
};

struct FunSignature;
struct SpecDeclaration;

struct SpecUse {
	ref<const SpecDeclaration> spec;
	DynArray<Type> type_arguments;
	SpecUse(ref<const SpecDeclaration> _spec, DynArray<Type> _type_arguments);
};

struct FunSignature {
	DynArray<TypeParameter> type_parameters;
	Type return_type;
	Identifier name;
	DynArray<Parameter> parameters;
	DynArray<SpecUse> specs;

	uint arity() const { return to_uint(parameters.size()); }
	bool is_generic() const {
		return !type_parameters.empty() || !specs.empty();
	}
};

class AnyBody {
public:
	enum class Kind {
		Nil, // not yet initialized
		Expr,
		CppSource,
	};
private:
	Kind _kind;

	union Data {
		ref<Expression> expression; // Ref so we don't have to depend on the definition of Expression here.
		ArenaString cpp_source;
		Data() {} // uninitialized
		~Data() {} // string freed by ~AnyBody
	};
	Data data;

public:
	AnyBody() : _kind(Kind::Nil) {}
	AnyBody(const AnyBody& other) { *this = other; }
	void operator=(const AnyBody& other) {
		_kind = other._kind;
		switch (other._kind) {
			case Kind::Nil:
				break;
			case Kind::Expr:
				data.expression = other.data.expression;
				break;
			case Kind::CppSource:
				data.cpp_source = other.data.cpp_source;
				break;
		}
	}
	AnyBody(ref<Expression> expression) : _kind(Kind::Expr) { data.expression = expression; }
	AnyBody(ArenaString cpp_source) : _kind(Kind::CppSource) { data.cpp_source = cpp_source; }

	Kind kind() const { return _kind; }
	const Expression& expression() const {
		assert(_kind == Kind::Expr);
		return data.expression;
	}
	const ArenaString& cpp_source() const {
		assert(_kind == Kind::CppSource);
		return data.cpp_source;
	}
};

struct FunDeclaration {
	ref<const Module> containing_module;
	FunSignature signature;
	AnyBody body;

	Identifier name() const { return signature.name; }
};

struct SpecDeclaration {
	ref<const Module> containing_module;
	DynArray<TypeParameter> type_parameters;
	Identifier name;
	DynArray<FunSignature> signatures;

	SpecDeclaration(ref<const Module> _containing_module, DynArray<TypeParameter> _type_parameters, Identifier _name)
		: containing_module(_containing_module), type_parameters(_type_parameters), name(_name) {}
};

// Within a single module, maps a struct name to declaration.
using StructsTable = Map<StringSlice, ref<const StructDeclaration>>;
// Within a single module, maps a spec name to declaration.
using SpecsTable = Map<StringSlice, ref<const SpecDeclaration>>;
// Within a single module, maps a fun name to the list of functions *in that module* with that name.
using FunsTable = MultiMap<StringSlice, ref<const FunDeclaration>>;
using StructsDeclarationOrder = Vec<ref<StructDeclaration>>;
using SpecsDeclarationOrder = Vec<ref<SpecDeclaration>>;
using FunsDeclarationOrder = Vec<ref<FunDeclaration>>;

// Note: if there is a parse error, this will just be empty.
struct Module {
	ArenaString path;
	Identifier name; // `For foo/bar/a.nz`, this is `a`.
	StructsDeclarationOrder structs_declaration_order;
	SpecsDeclarationOrder specs_declaration_order;
	FunsDeclarationOrder funs_declaration_order;
	StructsTable structs_table;
	SpecsTable specs_table;
	FunsTable funs_table;
	Vec<Diagnostic> diagnostics;

	Module(ArenaString _path, Identifier _name) : path(_path), name(_name) {}
};

struct CompiledProgram {
	Arena arena;
	Module module;
};

