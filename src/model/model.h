#pragma once

#include <cassert>
#include <cstdint> // uint64_t
#include <unordered_map>
#include <vector>

#include "../util/Alloc.h"
#include "../util/Map.h"
#include "../util/Option.h"
#include "../util/StringSlice.h"
#include "../diag/diag.h"

struct Identifier {
	ArenaString str;
	bool operator==(const Identifier& i) const { return str.slice() == i.str.slice(); }
	bool operator==(const StringSlice& s) const { return str == s; }
};
namespace std {
	template<>
	struct hash<Identifier> {
		size_t operator()(Identifier i) const {
			return hash<StringSlice>{}(i.str.slice());
		}
	};
}


enum class Effect { Pure, Get, Set, Io };
StringSlice effect_name(Effect e);

struct TypeParameter {
	Identifier name;
	//Index in the list of type parameters it appeared in.
	uint index;
	// TODO: constraints, and name might not exist if it's implicit
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

struct Module;

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

struct PlainType {
	Effect effect;
	InstStruct inst_struct;
};

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
	Type(Effect effect, InstStruct inst_struct) : _kind(Kind::Plain) {
		data.plain = PlainType { effect, inst_struct };
	}
	void operator=(const Type& other) {
		_kind = other._kind;
		switch (other._kind) {
			case Kind::Nil: break;
			case Kind::Plain: data.plain = other.data.plain; break;
			case Kind::Param: data.param = other.data.param; break;
		}
	}

	//Type(PlainType plain) : _kind(Kind::Plain) {
	//	data.plain = plain;
	//}
	Type(ref<const TypeParameter> param) : _kind(Kind::Param) {
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


struct StructField {
	Type type;
	Identifier name;
};

struct Parameter {
	Type type;
	Identifier name;
};

class Expression;
struct Fun;

struct StructFieldAccess {
	ref<Expression> target;
	ref<const StructField> field;
};

struct InstFun {
	ref<const Fun> fun;
	DynArray<Type> type_arguments;
};

struct Call {
	InstFun called;
	DynArray<Expression> arguments;
};

struct StructCreate {
	InstStruct inst_struct; // Effect is Io
	DynArray<Expression> arguments;
};

struct Let;
struct Seq;

struct Case;
struct When {
	DynArray<Case> cases;
	ref<Expression> elze;

	When(DynArray<Case> _cases, ref<Expression> _elze) : cases(_cases), elze(_elze) {
		assert(cases.size() != 0);
	}
};

class Expression {
public:
	enum class Kind {
		Nil,
		ParameterReference,
		LocalReference,
		StructFieldAccess,
		Let,
		Seq,
		Call,
		StructCreate,
		//This is the argument passed to a call to `literal`.
		StringLiteral,
		When,
	};

private:
	// Note: we are trying to keep this to 2 words. ref is one word.
	union Data {
		ref<const Parameter> parameter_reference;
		ref<const Let> local_reference;
		StructFieldAccess struct_field_access;
		ref<const Let> let;
		ref<const Seq> seq;
		Call call;
		StructCreate struct_create;
		ArenaString string_literal;
		When when;

		Data() {} // uninitialized
		~Data() {} // Nothing in here should need to be deleted
	};

	Kind _kind;
	Data data;

public:
	Kind kind() const { return _kind; }

	Expression() : _kind(Kind::Nil) {}

	Expression(const Expression& e) {
		_kind = Kind::Nil; // operator= asserts this
		*this = e;
	}

	Expression(ref<const Parameter> p) : _kind(Kind::ParameterReference) {
		data.parameter_reference = p;
	}

	Expression(ref<const Let> l, Kind kind) : _kind(kind) {
		if (kind == Kind::LocalReference) data.local_reference = l;
		else if (kind == Kind::Let) data.let = l;
		else assert(false);
	}

	Expression(ref<const Seq> seq) : _kind(Kind::Seq) {
		data.seq = seq;
	}

	Expression(StructFieldAccess s) : _kind(Kind::StructFieldAccess) {
		data.struct_field_access = s;
	}

	Expression(Call call) : _kind(Kind::Call) {
		data.call = call;
	}

	Expression(StructCreate create) : _kind(Kind::StructCreate) {
		data.struct_create = create;
	}

	explicit Expression(ArenaString value) : _kind(Kind::StringLiteral) {
		data.string_literal = value;
	}

	Expression(When when) : _kind(Kind::When) {
		data.when = when;
	}

	~Expression() {
		// Nothing to do, none have destructors
	}

	ref<const Parameter> parameter() const {
		assert(_kind == Kind::ParameterReference);
		return data.parameter_reference;
	}
	ref<const Let> local_reference() const {
		assert(_kind == Kind::LocalReference);
		return data.local_reference;
	}
	const StructFieldAccess& struct_field_access() const {
		assert(_kind == Kind::StructFieldAccess);
		return data.struct_field_access;
	}
	const Let& let() const {
		assert(_kind == Kind::Let);
		return *data.let;
	}
	const Seq& seq() const {
		assert(_kind == Kind::Seq);
		return *data.seq;
	}
	const Call& call() const {
		assert(_kind == Kind::Call);
		return data.call;
	}
	const StructCreate& struct_create() const {
		assert(_kind == Kind::StructCreate);
		return data.struct_create;
	}
	const ArenaString& string_literal() const {
		assert(_kind == Kind::StringLiteral);
		return data.string_literal;
	}
	const When& when() const {
		assert(_kind == Kind::When);
		return data.when;
	}

	void operator=(const Expression& e) {
		_kind = e._kind;
		switch (_kind) {
			case Kind::Nil:
				break;
			case Kind::ParameterReference:
				data.parameter_reference = e.data.parameter_reference;
				break;
			case Kind::LocalReference:
				data.local_reference = e.data.local_reference;
				break;
			case Kind::StructFieldAccess:
				data.struct_field_access = e.data.struct_field_access;
				break;
			case Kind::Let:
				data.let = e.data.let;
				break;
			case Kind::Seq:
				data.seq = e.data.seq;
				break;
			case Kind::Call:
				data.call = e.data.call;
				break;
			case Kind::StructCreate:
				data.struct_create = e.data.struct_create;
				break;
			case Kind::StringLiteral:
				data.string_literal = e.data.string_literal;
				break;
			case Kind::When:
				data.when = e.data.when;
				break;
		}
	}
};

struct Let {
	Type type;
	Identifier name;
	Expression init;
	Expression then;
	Let(const Let& other) = delete;
};

struct Seq {
	Expression first;
	Expression then;
	Seq(const Seq& other) = delete;
};

struct Case {
	Expression cond;
	Expression then;
};

struct AnyBody {
	enum class Kind {
		Nil, // not yet initialized
		Expression,
		CppSource,
	};
	Kind _kind;

	union Data {
		Expression expression;
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

	void operator=(Expression expression) {
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


// This is either an actual Fun declaration, or may be a synthetic fun created for calling a field of a struct.
struct Fun {
	ref<const Module> containing_module;
	DynArray<TypeParameter> type_parameters;
	Type return_type;
	Identifier name;
	DynArray<Parameter> parameters;
	AnyBody body;

	// Body filled in in a later pass.
	//Fun(ref<const Module> _containing_module, DynArray<TypeParameter> _type_parameters, Type _return_type, Identifier _name, DynArray<Parameter> _parameters)
	//	: containing_module(_containing_module), type_parameters(_type_parameters), return_type(_return_type), name(_name), parameters(_parameters) {}
	Fun(ref<const Module> _containing_module, DynArray<TypeParameter> _type_parameters) : containing_module(_containing_module), type_parameters(_type_parameters) {}
	Fun(const Fun& other) = delete;
	void operator=(const Fun& other) = delete;
	Fun(Fun&& other) = default;

	uint arity() const {
		return uint(parameters.size());
	}
};

// Group of functions with the same name.
struct OverloadGroup {
	std::vector<ref<const Fun>> funs;
};
using StructsTable = Map<StringSlice, ref<const StructDeclaration>>;
using FunsTable = Map<StringSlice, OverloadGroup>;

using StructsDeclarationOrder = std::vector<ref<StructDeclaration>>;
using FunsDeclarationOrder = std::vector<ref<Fun>>;

// Note: if there is a parse error, this will just be empty.
struct Module {
	ArenaString path;
	Identifier name; // `For foo/bar/a.nz`, this is `a`.
	StructsDeclarationOrder structs_declaration_order;
	FunsDeclarationOrder funs_declaration_order;
	StructsTable structs_table;
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
