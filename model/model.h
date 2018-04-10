#pragma once

#include <cassert>
#include <cstdint> // uint64_t

#include "../util/Alloc.h"
#include "../util/Option.h"
#include "../util/StringSlice.h"

struct Identifier {
//TODO: private:
public:
	ArenaString str;
	Identifier(ArenaString _str) : str(_str) {}
	operator StringSlice() const {
		return str;
	}
	StringSlice slice() const { return str; }
};

enum class Effect { Pure, Get, Set, Io };
const char* effect_name(Effect e);

struct TypeParameter {
	Identifier name;
	//Index in the list of type parameters it appeared in.
	uint index;
	// TODO: constraints, and name might not exist if it's implicit
};

struct StructField;

class StructBody {
	enum class Kind { Fields, CppName };
	union Data {
		DynArray<StructField> fields;
		ArenaString cpp_name; //TODO: we're not invoking the dtor, use a string ptr to an arena string

		Data() {} //uninitialized
		~Data() {}
	};
	Kind kind;
	Data data;

public:
	StructBody(DynArray<StructField> fields) : kind(Kind::Fields) {
		data.fields = fields;
	}
	StructBody(ArenaString cpp_name) : kind(Kind::CppName) {
		data.cpp_name = cpp_name;
	}

	bool is_fields() const { return kind == Kind::Fields; }
	DynArray<StructField>& fields() {
		assert(is_fields());
		return data.fields;
	}
	const DynArray<StructField>& fields() const {
		assert(is_fields());
		return data.fields;
	}
	const ArenaString& cpp_name() const {
		assert(kind == Kind::CppName);
		return data.cpp_name;
	}
};

struct StructDeclaration {
	Identifier name;
	DynArray<TypeParameter> type_parameters;
	StructBody body;

	StructDeclaration(Identifier _name, DynArray<TypeParameter> _type_parameters, DynArray<StructField> _fields)
		: name(_name), type_parameters(_type_parameters), body(_fields) {}
	StructDeclaration(Identifier _name, DynArray<TypeParameter> _type_parameters, ArenaString _cpp_name)
		: name(_name), type_parameters(_type_parameters), body(_cpp_name) {}
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
	enum Kind { Plain, Param };
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

	//TODO: shouldn't need?
	//StructField(Type type, StringSlice name) : type(type), name(name) {}
	StructField& operator=(StructField&& other) = default;
	//StructField(const StructField& other) = delete;
	//StructField(StructField&& other) = default;
};

struct Parameter {
	Type type;
	Identifier name;
	Parameter(Type _type, Identifier _name) : type(_type), name(_name) {}
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
		Let, // x = 1; x
		//TODO: local reference
				Call, // x factorial, x + y
		StructCreate,
		UintLiteral, // 0
		When,
	};

private:
	// Note: we are trying to keep this to 2 words. ref is one word.
	union Data {
		ref<const Parameter> parameter_reference;
		ref<const Let> local_reference;
		StructFieldAccess struct_field_access;
		ref<const Let> let;
		Call call;
		StructCreate struct_create;
		uint64_t uint_literal;
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

	Expression(StructFieldAccess s) : _kind(Kind::StructFieldAccess) {
		data.struct_field_access = s;
	}

	Expression(Call call) : _kind(Kind::Call) {
		data.call = call;
	}

	Expression(StructCreate create) : _kind(Kind::StructCreate) {
		data.struct_create = create;
	}

	Expression(uint64_t value) : _kind(Kind::UintLiteral) {
		data.uint_literal = value;
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
	const Call& call() const {
		assert(_kind == Kind::Call);
		return data.call;
	}
	const StructCreate& struct_create() const {
		assert(_kind == Kind::StructCreate);
		return data.struct_create;
	}
	const uint64_t& uint() const {
		assert(_kind == Kind::UintLiteral);
		return data.uint_literal;
	}
	const When& when() const {
		assert(_kind == Kind::When);
		return data.when;
	}

	void operator=(const Expression& e) {
		assert(_kind == Kind::Nil);
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
			case Kind::Call:
				data.call = e.data.call;
				break;
			case Kind::StructCreate:
				data.struct_create = e.data.struct_create;
				break;
			case Kind::UintLiteral:
				data.uint_literal = e.data.uint_literal;
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
	DynArray<TypeParameter> type_parameters;
	Type return_type;
	Identifier name;
	DynArray<Parameter> parameters;
	AnyBody body;

	// Body filled in in a later pass.
	Fun(DynArray<TypeParameter> _type_parameters, Type _return_type, Identifier _name, DynArray<Parameter> _parameters)
		: type_parameters(_type_parameters), return_type(_return_type), name(_name), parameters(_parameters) {}
	Fun(const Fun& other) = delete;
	void operator=(const Fun& other) = delete;
	Fun(Fun&& other) = default;

	uint arity() const {
		return uint(parameters.size());
	}

	bool is_generic() const {
		return !type_parameters.empty();
	}
};


using Structs = NonMovingCollection<StructDeclaration>;
using Funs = NonMovingCollection<Fun>;

struct Module {
	Structs structs;
	Funs funs;
	Arena arena;

	Module() = default;
	Module(const Module& other) = delete;
	Module(Module&& other) = default;
};
