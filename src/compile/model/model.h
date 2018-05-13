#pragma once

#include "../../util/store/Arena.h"
#include "../../util/store/ArenaString.h"
#include "../../util/store/Map.h"
#include "../../util/store/MultiMap.h"
#include "../../util/store/NonEmptyList.h"
#include "../../util/store/NonEmptyList_utils.h"
#include "../../util/assert.h"
#include "../../util/Path.h"
#include "../diag/SourceRange.h"
#include "./effect.h"

using Identifier = ArenaString;

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
		Slice<StructField> fields;
		ArenaString cpp_name;

		Data() {} //uninitialized
		~Data() {}
	};
	Kind _kind;
	Data data;

public:
	StructBody() : _kind{Kind::Nil} {}
	inline StructBody(const StructBody& other) { *this = other; }
	void operator=(const StructBody& other);
	inline StructBody(Slice<StructField> fields) : _kind{Kind::Fields} {
		data.fields = fields;
	}
	inline StructBody(ArenaString cpp_name) : _kind{Kind::CppName} {
		data.cpp_name = cpp_name;
	}

	Kind kind() const { return _kind; }
	inline bool is_fields() const { return _kind == StructBody::Kind::Fields; }
	inline Slice<StructField>& fields() {
		assert(_kind == Kind::Fields);
		return data.fields;
	}
	inline const Slice<StructField>& fields() const {
		assert(_kind == Kind::Fields);
		return data.fields;
	}
	inline const ArenaString& cpp_name() const {
		assert(_kind == Kind::CppName);
		return data.cpp_name;
	}
};

struct StructDeclaration {
	Ref<const Module> containing_module;
	Option<ArenaString> comment;
	SourceRange range;
	bool is_public;
	Slice<TypeParameter> type_parameters;
	Identifier name;
	bool copy;
	StructBody body;

	inline StructDeclaration(Ref<const Module> _containing_module, SourceRange _range, bool _is_public, Slice<TypeParameter> _type_parameters, Identifier _name, bool _copy)
		: containing_module{_containing_module}, range{_range}, is_public{_is_public}, type_parameters{_type_parameters}, name{_name}, copy{_copy} {}

	inline uint arity() const { return type_parameters.size(); }
};

class Type;

struct InstStruct {
	Ref<const StructDeclaration> strukt;
	Slice<Type> type_arguments;

	inline InstStruct(Ref<const StructDeclaration> _strukt, Slice<Type> _type_arguments) : strukt{_strukt}, type_arguments{_type_arguments} {
		assert(type_arguments.size() == strukt->type_parameters.size());
	}
	bool is_deeply_concrete() const;

	struct hash_deeply_concrete {
		hash_t operator()(const InstStruct& i);
	};
};

class StoredType {
public:
	enum class Kind { Nil, Bogus, InstStruct, TypeParameter };
private:
	union Data {
		InstStruct inst_struct;
		Ref<const TypeParameter> param;
		Data() {} // uninitialized
		~Data() {}
	};
	Kind _kind;
	Data data;

	StoredType(bool _dummy __attribute__((unused))) : _kind{Kind::Bogus} {}

public:
	inline Kind kind() const { return _kind; }

	inline StoredType() : _kind{Kind::Nil} {}
	inline static StoredType bogus() { return StoredType { false }; }
	inline StoredType(const StoredType& other) { *this = other;  }
	void operator=(const StoredType& other);

	inline explicit StoredType(InstStruct i) : _kind{Kind::InstStruct} {
		data.inst_struct = i;
	}
	inline explicit StoredType(Ref<const TypeParameter> param) : _kind{Kind::TypeParameter} {
		data.param = param;
	}

	inline bool is_bogus() const { return _kind == Kind::Bogus; }
	inline bool is_inst_struct() const { return _kind == Kind::InstStruct; }
	inline bool is_type_parameter() const { return _kind == Kind::TypeParameter; }

	inline const InstStruct& inst_struct() const {
		assert(_kind == Kind::InstStruct);
		return data.inst_struct;
	}

	inline Ref<const TypeParameter> param() const {
		assert(_kind == Kind::TypeParameter);
		return data.param;
	}
};

struct Parameter;

class Lifetime {
	// If this is '0', the value is 'new'.
	// In a struct field, that means stored by value.
	// In a return type, that means a new value is written to an output pointer.
	// In a parameter, that means a closure is passed that can write a new value.
	enum class Flags : unsigned short {
		None = 0,
		Ret = 1 << 0,
		Loc = 1 << 1,
		Param0 = 1 << 2,
		Param1 = 1 << 3,
		Param2 = 1 << 4,
		Param3 = 1 << 5,
		//TODO: LifetimeVar0 = 1 << 6, etc
	};
	inline static Flags flags_union(Flags a, Flags b) {
		return static_cast<Flags>(static_cast<unsigned short>(a) | static_cast<unsigned short>(b));
	}
	inline static Flags flags_intersection(Flags a, Flags b) {
		return static_cast<Flags>(static_cast<unsigned short>(a) & static_cast<unsigned short>(b));
	}

	//TODO: Effect effect; (note If 'new', borrows should be empty...)
	// An expression may have multiple borrows, as in `e = cond ? a : b` having the combined borrows of `a` and `b`
	// If this is empty, then it is 'new'.
	Flags _flags;

	Lifetime(Flags flags) : _flags{flags} {}

	inline bool has(Flags flag) const {
		return flags_intersection(_flags, flag) != Flags::None;
	}

	inline static Flags flag_of_parameter(uint index) {
		switch (index) {
			case 0: return Flags::Param0;
			case 1: return Flags::Param1;
			case 2: return Flags::Param2;
			case 3: return Flags::Param3;
			default: todo();
		}

	}

public:
	inline static Lifetime noborrow() { return { {} }; }
	inline static Lifetime local_borrow() {
		return { Flags::Loc };
	}
	inline static Lifetime of_parameter(uint index) {
		return { flag_of_parameter(index) };
	}

	inline bool is_borrow() const {
		//TODO: effect should agree
		return _flags != Flags::None;
	}

	inline bool has_ret() const { return has(Flags::Ret); }
	inline bool has_loc() const { return has(Flags::Loc); }
	inline bool has_parameter(uint index) const {
		return has(flag_of_parameter(index));
	}
	inline bool has_lifetime_variables() const { return false; } // TODO

	inline bool exactly_same_as(const Lifetime& other) const {
		return _flags == other._flags;
	}

	class Builder {
		Flags flags = Flags::None;

	public:
		void add(Lifetime l) {
			flags = flags_union(flags, l._flags);
		}

		Lifetime finish() {
			return { flags };
		}
	};
};

class Type {
private:
	StoredType _stored_type;
	Late<Lifetime> _lifetime;

	inline bool is_valid() const {
		switch (_stored_type.kind()) {
			case StoredType::Kind::Nil:
			case StoredType::Kind::Bogus:
				return false;
			case StoredType::Kind::InstStruct:
			case StoredType::Kind::TypeParameter:
				return true;
		}
	}

public:
	inline Type() {}
	inline Type(StoredType s, Lifetime lifetime) : _stored_type{s}, _lifetime{lifetime} {}

	inline const Lifetime& lifetime() const {
		return _lifetime.get();
	}

	inline bool is_bogus() const {
		switch (_stored_type.kind()) {
			case StoredType::Kind::Nil:
				unreachable();
			case StoredType::Kind::Bogus:
				return true;
			case StoredType::Kind::InstStruct:
			case StoredType::Kind::TypeParameter:
				return false;
		}
	}

	inline static Type noborrow(StoredType s) {
		return Type { s, Lifetime::noborrow() };
	}

	inline const StoredType& stored_type() const {
		assert(is_valid());
		return _stored_type;
	}

	inline const StoredType& stored_type_or_bogus() const {
		assert(_stored_type.kind() != StoredType::Kind::Nil);
		return _stored_type;
	}
};

struct StructField {
	Option<ArenaString> comment;
	Type type;
	Identifier name;
};

struct Parameter {
	Identifier name;
	Type type;
	uint index;
};

struct FunSignature;
struct SpecDeclaration;

struct SpecUse {
	Ref<const SpecDeclaration> spec;
	Slice<Type> type_arguments;
	SpecUse(Ref<const SpecDeclaration> _spec, Slice<Type> _type_arguments);
};

struct FunSignature {
	Option<ArenaString> comment;
	Slice<TypeParameter> type_parameters;
	Type return_type;
	Identifier name;
	Slice<Parameter> parameters;
	Slice<SpecUse> specs;

	inline FunSignature(Option<ArenaString> _comment, Slice<TypeParameter> _type_parameters, Type _return_type, Identifier _name, Slice<Parameter> _parameters, Slice<SpecUse> _specs)
		: comment(_comment), type_parameters(_type_parameters), return_type(_return_type), name(_name), parameters(_parameters), specs(_specs) {}
	inline FunSignature(Identifier _name) : name{_name} {}

	inline uint arity() const { return parameters.size(); }
	bool is_generic() const;
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
		Ref<Expression> expression; // Ref so we don't have to depend on the definition of Expression here.
		ArenaString cpp_source;
		Data() {} // uninitialized
		~Data() {} // string freed by ~AnyBody
	};
	Data data;

public:
	inline AnyBody() : _kind{Kind::Nil} {}
	inline AnyBody(const AnyBody& other) { *this = other; }
	void operator=(const AnyBody& other);
	inline AnyBody(Ref<Expression> expression) : _kind{Kind::Expr} { data.expression = expression; }
	inline AnyBody(ArenaString cpp_source) : _kind{Kind::CppSource} { data.cpp_source = cpp_source; }

	inline Kind kind() const { return _kind; }
	inline Expression& expression() {
		assert(_kind == Kind::Expr);
		return data.expression;
	}
	inline const Expression& expression() const {
		assert(_kind == Kind::Expr);
		return data.expression;
	}
	inline const ArenaString& cpp_source() const {
		assert(_kind == Kind::CppSource);
		return data.cpp_source;
	}
};

struct FunDeclaration {
	Ref<const Module> containing_module;
	bool is_public;
	FunSignature signature;
	AnyBody body;

	inline Identifier name() const { return signature.name; }
};

struct SpecDeclaration {
	Ref<const Module> containing_module;
	SourceRange range;
	Option<ArenaString> comment;
	bool is_public;
	Slice<TypeParameter> type_parameters;
	Identifier name;
	Slice<FunSignature> signatures;

	inline SpecDeclaration(Ref<const Module> _containing_module, SourceRange _range, Option<ArenaString> _comment, bool _is_public, Slice<TypeParameter> _type_parameters, Identifier _name)
		: containing_module(_containing_module), range(_range), comment(_comment), is_public(_is_public), type_parameters(_type_parameters), name(_name) {}
};

// Within a single module, maps a struct name to declaration.
using StructsTable = Map<StringSlice, Ref<const StructDeclaration>, StringSlice::hash>;
// Within a single module, maps a spec name to declaration.
using SpecsTable = Map<StringSlice, Ref<const SpecDeclaration>, StringSlice::hash>;
// Within a single module, maps a fun name to the list of functions *in that module* with that name.
using FunsTable = MultiMap<StringSlice, Ref<const FunDeclaration>, StringSlice::hash>;
using StructsDeclarationOrder = Slice<StructDeclaration>;
using SpecsDeclarationOrder = Slice<SpecDeclaration>;
using FunsDeclarationOrder = Slice<FunDeclaration>;

// Note: if there is a parse error, this will just be empty.
struct Module {
	Path path;
	Slice<Ref<const Module>> imports;
	Option<ArenaString> comment;
	StructsDeclarationOrder structs_declaration_order;
	SpecsDeclarationOrder specs_declaration_order;
	FunsDeclarationOrder funs_declaration_order;
	StructsTable structs_table;
	SpecsTable specs_table;
	FunsTable funs_table;

	inline StringSlice name() const { return path.base_name(); }
};
