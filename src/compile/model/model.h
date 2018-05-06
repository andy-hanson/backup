#pragma once

#include "../../util/store/Arena.h"
#include "../../util/store/ArenaString.h"
#include "../../util/store/Map.h"
#include "../../util/store/MultiMap.h"
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
	StructBody() : _kind(Kind::Nil) {}
	inline StructBody(const StructBody& other) { *this = other; }
	void operator=(const StructBody& other);
	inline StructBody(Slice<StructField> fields) : _kind(Kind::Fields) {
		data.fields = fields;
	}
	inline StructBody(ArenaString cpp_name) : _kind(Kind::CppName) {
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
		: containing_module(_containing_module), range(_range), is_public(_is_public), type_parameters(_type_parameters), name(_name), copy(_copy) {}

	inline uint arity() const { return type_parameters.size(); }
};

class Type;

struct InstStruct {
	Ref<const StructDeclaration> strukt;
	Slice<Type> type_arguments;

	inline InstStruct(Ref<const StructDeclaration> _strukt, Slice<Type> _type_arguments) : strukt(_strukt), type_arguments(_type_arguments) {
		assert(type_arguments.size() == strukt->type_parameters.size());
	}
	bool is_deeply_concrete() const;

	struct hash {
		hash_t operator()(const InstStruct& i);
	};
};
bool operator==(const InstStruct& a, const InstStruct& b);

class Type {
public:
	enum class Kind { Nil, Bogus, InstStruct, Param };
private:
	union Data {
		InstStruct inst_struct;
		Ref<const TypeParameter> param;
		Data() {} // uninitialized
		~Data() {}
	};
	Kind _kind;
	Data data;

	Type(bool dummy __attribute__((unused))) : _kind(Kind::Bogus) {}

public:
	inline Kind kind() const { return _kind; }

	inline Type() : _kind(Kind::Nil) {}
	static Type bogus() { return Type { false }; }
	inline Type(const Type& other) { *this = other;  }
	void operator=(const Type& other);

	inline explicit Type(InstStruct i) : _kind(Kind::InstStruct) {
		data.inst_struct = i;
	}
	inline explicit Type(Ref<const TypeParameter> param) : _kind(Kind::Param) {
		data.param = param;
	}

	inline bool is_inst_struct() const { return _kind == Kind::InstStruct; }
	inline bool is_parameter() const { return _kind == Kind::Param; }

	inline const InstStruct& inst_struct() const {
		assert(_kind == Kind::InstStruct);
		return data.inst_struct;
	}

	inline Ref<const TypeParameter> param() const {
		assert(_kind == Kind::Param);
		return data.param;
	}

	struct hash {
		hash_t operator()(const Type& t) const;
	};
};

bool operator==(const Type& a, const Type& b);
inline bool operator!=(const Type& a, const Type& b) { return !(a == b); }

struct StructField {
	Option<ArenaString> comment;
	Type type;
	Identifier name;
};

struct Parameter {
	bool from;
	Effect effect;
	Type type;
	Identifier name;
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
	Effect effect; // Return effect will be the worst of this, and the effects of all 'from' parameters
	Type return_type;
	Identifier name;
	Slice<Parameter> parameters;
	Slice<SpecUse> specs;

	inline FunSignature(Option<ArenaString> _comment, Slice<TypeParameter> _type_parameters, Effect _effect, Type _return_type, Identifier _name, Slice<Parameter> _parameters, Slice<SpecUse> _specs)
		: comment(_comment), type_parameters(_type_parameters), effect(_effect), return_type(_return_type), name(_name), parameters(_parameters), specs(_specs) {}
	inline FunSignature(Identifier _name) : name(_name) {}

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
	inline AnyBody() : _kind(Kind::Nil) {}
	inline AnyBody(const AnyBody& other) { *this = other; }
	void operator=(const AnyBody& other);
	inline AnyBody(Ref<Expression> expression) : _kind(Kind::Expr) { data.expression = expression; }
	inline AnyBody(ArenaString cpp_source) : _kind(Kind::CppSource) { data.cpp_source = cpp_source; }

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
