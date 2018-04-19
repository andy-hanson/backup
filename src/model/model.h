#pragma once

#include <cassert>
#include <vector>

#include "../util/Alloc.h"
#include "../util/hash_util.h"
#include "../util/Map.h"
#include "../util/Option.h"
#include "../diag/diag.h"
#include "./Identifier.h"
#include "../util/collection_util.h"

class Expression;

struct Module;


//region Type

enum class Effect { Pure, Get, Set, Io };
StringSlice effect_name(Effect e);

struct TypeParameter {
	Identifier name;
	uint index;
};

struct StructField;

class StructBody {
public:
	enum class Kind { Fields, CppName, CppBody };
private:
	union Data {
		DynArray<StructField> fields;
		ArenaString cpp_name_or_body;

		Data() {} //uninitialized
		~Data() {}
	};
	Kind _kind;
	Data data;

public:
	Kind kind() const { return _kind; }
	bool is_fields() const { return _kind == StructBody::Kind::Fields; }

	StructBody(const StructBody& other) {
		*this = other;
	}
	void operator=(const StructBody& other) {
		_kind = other._kind;
		switch (_kind) {
			case Kind::Fields:
				data.fields = other.data.fields;
				break;
			case Kind::CppName:
			case Kind::CppBody:
				data.cpp_name_or_body = other.data.cpp_name_or_body;
				break;
		}
	}
	StructBody(DynArray<StructField> fields) : _kind(Kind::Fields) {
		data.fields = fields;
	}
	StructBody(Kind kind, ArenaString cpp_name_or_body) : _kind(kind) {
		assert(_kind == Kind::CppName || _kind == Kind::CppBody);
		data.cpp_name_or_body = cpp_name_or_body;
	}

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
		return data.cpp_name_or_body;
	}
	const ArenaString& cpp_body() const {
		assert(_kind == Kind::CppBody);
		return data.cpp_name_or_body;
	}
};

struct StructDeclaration {
	ref<const Module> containing_module;
	Identifier name;
	DynArray<TypeParameter> type_parameters;
	StructBody body;

	StructDeclaration(ref<const Module> _containing_module, Identifier _name, DynArray<TypeParameter> _type_parameters)
		: containing_module(_containing_module), name(_name), type_parameters(_type_parameters), body({}) {}
	StructDeclaration(const StructDeclaration& other) = delete;

	size_t arity() const {
		return type_parameters.size();
	}
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
	//TODO:Kill?
	Type(const Type& other) {
		*this = other;
	}
	explicit Type(PlainType p) : _kind(Kind::Plain) {
		data.plain = p;
	}
	void operator=(const Type& other) {
		_kind = other._kind;
		switch (other._kind) {
			case Kind::Nil: break;
			case Kind::Plain: data.plain = other.data.plain; break;
			case Kind::Param: data.param = other.data.param; break;
		}
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

//endregion

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

	FunSignature(DynArray<TypeParameter> _type_parameters) : type_parameters(_type_parameters) {}

	uint arity() const { return to_uint(parameters.size()); }
	bool is_generic() const {
		return !type_parameters.empty() || !specs.empty();
	}
};

struct AnyBody {
	enum class Kind {
		Nil, // not yet initialized
		Expression,
		CppSource,
	};
	Kind _kind;

	union Data {
		ref<Expression> expression; // Ref so we don't have to depend on the definition of Expression here.
		ArenaString cpp_source;
		Data() {} // uninitialized
		~Data() {} // string freed by ~AnyBody
	};
	Data data;

public:
	Kind kind() const { return _kind; }

	AnyBody() : _kind(Kind::Nil) {}

	const Expression& expression() const {
		assert(_kind == Kind::Expression);
		return data.expression;
	}

	const ArenaString& cpp_source() const {
		assert(_kind == Kind::CppSource);
		return data.cpp_source;
	}

	AnyBody(const AnyBody& other) : _kind(other._kind) {
		switch (other._kind) {
			case Kind::Nil:
				break;
			case Kind::Expression:
				data.expression = other.data.expression;
				break;
			case Kind::CppSource:
				data.cpp_source = other.data.cpp_source;
				break;
		}
	}

	void operator=(ref<Expression> expression) {
		assert(_kind == Kind::Nil);
		_kind = Kind::Expression;
		data.expression = expression;
	}

	void operator=(ArenaString cpp_source) {
		assert(_kind == Kind::Nil);
		_kind = Kind::CppSource;
		data.cpp_source = cpp_source;
	}
};

struct FunDeclaration {
	ref<const Module> containing_module;
	FunSignature signature;
	AnyBody body;

	Identifier name() const { return signature.name; }

	// Rest filled in in future passes.
	FunDeclaration(ref<const Module> _containing_module, DynArray<TypeParameter> _type_parameters) : containing_module(_containing_module), signature(_type_parameters) {}
	FunDeclaration(const FunDeclaration& other) = delete;
	void operator=(const FunDeclaration& other) = delete;
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
using StructsDeclarationOrder = std::vector<ref<StructDeclaration>>;
using SpecsDeclarationOrder = std::vector<ref<SpecDeclaration>>;
using FunsDeclarationOrder = std::vector<ref<FunDeclaration>>;

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
	List<Diagnostic> diagnostics;

	Module(ArenaString _path, Identifier _name) : path(_path), name(_name) {}

	// Structs and funs will point to this module.
	Module(const Module& other) = delete;
};

struct CompiledProgram {
	Arena arena;
	Module module; //TODO: multiple modules
};
