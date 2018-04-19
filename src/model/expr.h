#pragma once

#include "../util/Alloc.h"
#include "./model.h"

class Expression;
struct StructFieldAccess {
	ref<Expression> target;
	ref<const StructField> field;
};

struct SpecUseSig {
	ref<const SpecUse> spec_use;
	ref<const FunSignature> signature;

	SpecUseSig(ref<const SpecUse> _spec_use, ref<const FunSignature> _signature) : spec_use(_spec_use), signature(_signature) {
		assert(spec_use->spec->signatures.contains_ref(signature));
	}
};

/**
The theing being called during overload resolution.
Note that are still inferring the type arguments for the function.
But SigUse will have filled-in type arguments for the *sigs*'s type parameters, which are different from the function's type parameters.
*/
class CalledDeclaration {
public:
	enum class Kind { Fun, Spec };
private:
	union Data {
		ref<const FunDeclaration> fun_decl;
		SpecUseSig spec_use_sig;

		Data() {}
		~Data() {}
	};
	Kind _kind;
	Data data;

public:
	CalledDeclaration(ref<const FunDeclaration> fun_decl) : _kind(Kind::Fun) {
		data.fun_decl = fun_decl;
	}
	CalledDeclaration(SpecUseSig spec) : _kind(Kind::Spec) {
		data.spec_use_sig = spec;
	}
	CalledDeclaration(const CalledDeclaration& other) {
		*this = other;
	}
	void operator=(const CalledDeclaration& other) {
		_kind = other._kind;
		switch (_kind) {
			case Kind::Fun: data.fun_decl = other.data.fun_decl; break;
			case Kind::Spec: data.spec_use_sig = other.data.spec_use_sig; break;
		}
	}

	const FunSignature& sig() {
		switch (_kind) {
			case Kind::Fun:
				return fun()->signature;
			case Kind::Spec:
				return spec().signature;
		}
	}

	Kind kind() const { return _kind; }

	ref<const FunDeclaration> fun() const {
		assert(_kind == Kind::Fun);
		return data.fun_decl;
	}

	const SpecUseSig& spec() const {
		assert(_kind == Kind::Spec);
		return data.spec_use_sig;
	}
};

struct Called {
	CalledDeclaration called_declaration;
	DynArray<Type> type_arguments;
	// These will be in order of the signatures from the specs of the function we're calling.
	// For each spec, there is an array of a Called matching each signature in that spec.
	// Note: if there is a generic spec signature, it should be matched by a generic function. And a non-generic spec signature can't be matched by a generic function.
	// So this uses CalledDeclaration and not Called.
	DynArray<DynArray<CalledDeclaration>> spec_impls;
};

struct Call {
	Called called;
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
