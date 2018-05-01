#pragma once

#include <cassert>

#include "../../util/Alloc.h"
#include "../../util/Map.h"
#include "../../util/MultiMap.h"

#include "./effect.h"
#include "../../host/Path.h"

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
		Arr<StructField> fields;
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
	inline StructBody(Arr<StructField> fields) : _kind(Kind::Fields) {
		data.fields = fields;
	}
	inline StructBody(ArenaString cpp_name) : _kind(Kind::CppName) {
		data.cpp_name = cpp_name;
	}

	Kind kind() const { return _kind; }
	inline bool is_fields() const { return _kind == StructBody::Kind::Fields; }
	inline Arr<StructField>& fields() {
		assert(_kind == Kind::Fields);
		return data.fields;
	}
	inline const Arr<StructField>& fields() const {
		assert(_kind == Kind::Fields);
		return data.fields;
	}
	inline const ArenaString& cpp_name() const {
		assert(_kind == Kind::CppName);
		return data.cpp_name;
	}
};

struct StructDeclaration {
	ref<const Module> containing_module;
	Option<ArenaString> comment;
	SourceRange range;
	bool is_public;
	Arr<TypeParameter> type_parameters;
	Identifier name;
	bool copy;
	StructBody body;

	inline StructDeclaration(ref<const Module> _containing_module, SourceRange _range, bool _is_public, Arr<TypeParameter> _type_parameters, Identifier _name, bool _copy)
		: containing_module(_containing_module), range(_range), is_public(_is_public), type_parameters(_type_parameters), name(_name), copy(_copy) {}

	inline size_t arity() const { return type_parameters.size(); }
};

class Type;

struct InstStruct {
	ref<const StructDeclaration> strukt;
	Arr<Type> type_arguments;

	inline InstStruct(ref<const StructDeclaration> _strukt, Arr<Type> _type_arguments) : strukt(_strukt), type_arguments(_type_arguments) {
		assert(type_arguments.size() == strukt->type_parameters.size());
	}
	bool is_deeply_concrete() const;

	struct hash {
		size_t operator()(const InstStruct& i);
	};
};
bool operator==(const InstStruct& a, const InstStruct& b);

class Type {
public:
	enum class Kind { Nil, Bogus, InstStruct, Param };
private:
	union Data {
		InstStruct inst_struct;
		ref<const TypeParameter> param;
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
	inline explicit Type(ref<const TypeParameter> param) : _kind(Kind::Param) {
		data.param = param;
	}

	inline bool is_inst_struct() const { return _kind == Kind::InstStruct; }
	inline bool is_parameter() const { return _kind == Kind::Param; }

	inline const InstStruct& inst_struct() const {
		assert(_kind == Kind::InstStruct);
		return data.inst_struct;
	}

	inline ref<const TypeParameter> param() const {
		assert(_kind == Kind::Param);
		return data.param;
	}

	struct hash {
		size_t operator()(const Type& t) const;
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
	ref<const SpecDeclaration> spec;
	Arr<Type> type_arguments;
	SpecUse(ref<const SpecDeclaration> _spec, Arr<Type> _type_arguments);
};

struct FunSignature {
	Option<ArenaString> comment;
	Arr<TypeParameter> type_parameters;
	Effect effect; // Return effect will be the worst of this, and the effects of all 'from' parameters
	Type return_type;
	Identifier name;
	Arr<Parameter> parameters;
	Arr<SpecUse> specs;

	inline FunSignature(Option<ArenaString> _comment, Arr<TypeParameter> _type_parameters, Effect _effect, Type _return_type, Identifier _name, Arr<Parameter> _parameters, Arr<SpecUse> _specs)
		: comment(_comment), type_parameters(_type_parameters), effect(_effect), return_type(_return_type), name(_name), parameters(_parameters), specs(_specs) {}
	inline FunSignature(Identifier _name) : name(_name) {}

	inline uint arity() const { return to_uint(parameters.size()); }
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
		ref<Expression> expression; // Ref so we don't have to depend on the definition of Expression here.
		ArenaString cpp_source;
		Data() {} // uninitialized
		~Data() {} // string freed by ~AnyBody
	};
	Data data;

public:
	inline AnyBody() : _kind(Kind::Nil) {}
	inline AnyBody(const AnyBody& other) { *this = other; }
	void operator=(const AnyBody& other);
	inline AnyBody(ref<Expression> expression) : _kind(Kind::Expr) { data.expression = expression; }
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
	ref<const Module> containing_module;
	bool is_public;
	FunSignature signature;
	AnyBody body;

	inline Identifier name() const { return signature.name; }
};

struct SpecDeclaration {
	ref<const Module> containing_module;
	SourceRange range;
	Option<ArenaString> comment;
	bool is_public;
	Arr<TypeParameter> type_parameters;
	Identifier name;
	Arr<FunSignature> signatures;

	inline SpecDeclaration(ref<const Module> _containing_module, SourceRange _range, Option<ArenaString> _comment, bool _is_public, Arr<TypeParameter> _type_parameters, Identifier _name)
		: containing_module(_containing_module), range(_range), comment(_comment), is_public(_is_public), type_parameters(_type_parameters), name(_name) {}
};

// Within a single module, maps a struct name to declaration.
using StructsTable = Map<StringSlice, ref<const StructDeclaration>, StringSlice::hash>;
// Within a single module, maps a spec name to declaration.
using SpecsTable = Map<StringSlice, ref<const SpecDeclaration>, StringSlice::hash>;
// Within a single module, maps a fun name to the list of functions *in that module* with that name.
using FunsTable = MultiMap<StringSlice, ref<const FunDeclaration>, StringSlice::hash>;
using StructsDeclarationOrder = Arr<StructDeclaration>;
using SpecsDeclarationOrder = Arr<SpecDeclaration>;
using FunsDeclarationOrder = Arr<FunDeclaration>;

// Note: if there is a parse error, this will just be empty.
struct Module {
	Path path;
	Arr<ref<const Module>> imports;
	Option<ArenaString> comment;
	StructsDeclarationOrder structs_declaration_order;
	SpecsDeclarationOrder specs_declaration_order;
	FunsDeclarationOrder funs_declaration_order;
	StructsTable structs_table;
	SpecsTable specs_table;
	FunsTable funs_table;

	inline Module(Path _path, Arr<ref<const Module>> _imports, Option<ArenaString> _comment) : path(_path), imports(_imports), comment(_comment) {}

	inline Identifier name() const { return path.base_name(); }
};
