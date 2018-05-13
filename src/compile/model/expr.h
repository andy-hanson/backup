#pragma once

#include "../../util/store/Arena.h"
#include "./model.h"

class Expression;
struct StructFieldAccess {
	Type accessed_field_type; // TODO: compute on demand?
	Ref<Expression> target;
	Ref<const StructField> field;
};

struct SpecUseSig {
	Ref<const SpecUse> spec_use;
	Ref<const FunSignature> signature;

	inline SpecUseSig(Ref<const SpecUse> _spec_use, Ref<const FunSignature> _signature) : spec_use{_spec_use}, signature{_signature} {
		assert(spec_use->spec->signatures.contains_ref(signature));
	}
};

/**
The thing being called during overload resolution.
Note that are still inferring the type arguments for the function.
But SigUse will have filled-in type arguments for the *sigs*'s type parameters, which are different from the function's type parameters.
*/
class CalledDeclaration {
public:
	enum class Kind { Fun, Spec };
private:
	union Data {
		Ref<const FunDeclaration> fun_decl;
		SpecUseSig spec_use_sig;

		Data() {}
		~Data() {}
	};
	Kind _kind;
	Data data;

public:
	explicit CalledDeclaration(Ref<const FunDeclaration> fun_decl) : _kind{Kind::Fun} {
		data.fun_decl = fun_decl;
	}
	explicit CalledDeclaration(SpecUseSig spec) : _kind{Kind::Spec} {
		data.spec_use_sig = spec;
	}
	inline CalledDeclaration(const CalledDeclaration& other) { *this = other; }
	void operator=(const CalledDeclaration& other);

	const FunSignature& sig() const;

	inline Kind kind() const { return _kind; }
	inline Ref<const FunDeclaration> fun() const {
		assert(_kind == Kind::Fun);
		return data.fun_decl;
	}
	inline const SpecUseSig& spec() const {
		assert(_kind == Kind::Spec);
		return data.spec_use_sig;
	}
};

struct Called {
	CalledDeclaration called_declaration;
	Slice<Type> type_arguments;
	// These will be in order of the signatures from the specs of the function we're calling.
	// For each spec, there is an array of a Called matching each signature in that spec.
	// Note: if there is a generic spec signature, it should be matched by a generic function. And a non-generic spec signature can't be matched by a generic function.
	// So this uses CalledDeclaration and not Called.
	Slice<Slice<CalledDeclaration>> spec_impls;
};

struct Call {
	Type concrete_return_type; //TODO: compute on demand instead of storing eagerly?
	Called called;
	Slice<Expression> arguments;
};

struct StructCreate {
	InstStruct inst_struct; // Effect is Io
	Slice<Expression> arguments;
};

struct Let;
struct Seq;

struct Case;
struct When {
	Slice<Case> cases;
	Ref<Expression> elze;
	// Common type of all cases and elze.
	Type type; // TODO: compute on demand?

	inline When(Slice<Case> _cases, Ref<Expression> _elze, Type _type) : cases{_cases}, elze{_elze}, type{_type} {
		assert(cases.size() != 0);
	}
};

class Expression {
public:
	enum class Kind {
		Nil,
		Bogus, // A compile error prevented us from creating a valid Expression.
		ParameterReference,
		LocalReference,
		StructFieldAccess,
		Let,
		Seq,
		Call,
		StructCreate,
		// This is the argument passed to a call to `literal`.
		StringLiteral,
		When,
		Assert,
		Pass,
	};

private:
	// Note: we are trying to keep this to 2 words. ref is one word.
	union Data {
		Ref<const Parameter> parameter_reference;
		Ref<const Let> local_reference;
		StructFieldAccess struct_field_access;
		Ref<Let> let;
		Ref<Seq> seq;
		Call call;
		StructCreate struct_create;
		ArenaString string_literal;
		When when;
		Ref<Expression> asserted;

		Data() {} // uninitialized
		~Data() {} // Nothing in here should need to be deleted
	};

	Kind _kind;
	Data data;

public:
	inline Kind kind() const { return _kind; }

	Expression() : _kind{Kind::Nil} {}

	inline static Expression bogus() {
		return Expression { Kind::Bogus };
	}

	inline Expression(const Expression& e) {
		_kind = Kind::Nil; // operator= asserts this
		*this = e;
	}

	explicit Expression(Ref<const Parameter> p) : _kind{Kind::ParameterReference} {
		data.parameter_reference = p;
	}

	inline Expression(Ref<const Let> l, Kind kind) : _kind{kind} {
		assert(kind == Kind::LocalReference);
		data.local_reference = l;
	}

	inline Expression(Ref<Let> l, Kind kind) : _kind{kind} {
		assert(kind == Kind::Let);
		data.let = l;
	}

	inline explicit Expression(Ref<Seq> seq) : _kind{Kind::Seq} {
		data.seq = seq;
	}

	inline explicit Expression(StructFieldAccess s) : _kind{Kind::StructFieldAccess} {
		data.struct_field_access = s;
	}

	inline explicit Expression(Call call) : _kind{Kind::Call} {
		data.call = call;
	}

	inline explicit Expression(StructCreate create) : _kind{Kind::StructCreate} {
		data.struct_create = create;
	}

	inline explicit Expression(ArenaString value) : _kind{Kind::StringLiteral} {
		data.string_literal = value;
	}

	inline explicit Expression(When when) : _kind{Kind::When} {
		data.when = when;
	}

	Expression(Ref<Expression> asserted, Kind kind) : _kind{kind} {
		assert(kind == Kind::Assert);
		data.asserted = asserted;
	}

	Expression(Kind kind) : _kind{Kind::Pass} {
		assert(kind == Kind::Pass || kind == Kind::Bogus);
	}

	~Expression() {
		// Nothing to do, none have destructors
	}

	inline Ref<const Parameter> parameter_reference() const {
		assert(_kind == Kind::ParameterReference);
		return data.parameter_reference;
	}
	inline Ref<const Let> local_reference() const {
		assert(_kind == Kind::LocalReference);
		return data.local_reference;
	}
	inline StructFieldAccess& struct_field_access() {
		assert(_kind == Kind::StructFieldAccess);
		return data.struct_field_access;
	}
	inline const StructFieldAccess& struct_field_access() const {
		assert(_kind == Kind::StructFieldAccess);
		return data.struct_field_access;
	}
	inline Let& let() {
		assert(_kind == Kind::Let);
		return *data.let;
	}
	inline const Let& let() const {
		assert(_kind == Kind::Let);
		return *data.let;
	}
	inline Seq& seq() {
		assert(_kind == Kind::Seq);
		return *data.seq;
	}
	inline const Seq& seq() const {
		assert(_kind == Kind::Seq);
		return *data.seq;
	}
	inline Call& call() {
		assert(_kind == Kind::Call);
		return data.call;
	}
	inline const Call& call() const {
		assert(_kind == Kind::Call);
		return data.call;
	}
	inline const StructCreate& struct_create() const {
		assert(_kind == Kind::StructCreate);
		return data.struct_create;
	}
	inline const ArenaString& string_literal() const {
		assert(_kind == Kind::StringLiteral);
		return data.string_literal;
	}
	inline const When& when() const {
		assert(_kind == Kind::When);
		return data.when;
	}
	inline Expression& asserted() {
		assert(_kind == Kind::Assert);
		return data.asserted;
	}
	inline const Expression& asserted() const {
		assert(_kind == Kind::Assert);
		return data.asserted;
	}

	void operator=(const Expression& e);
};

struct Let {
	// Type of 'name', not of the final expression.
	// Note that during type-checking, the
	Type type;
	Identifier name;
	Expression init;
	Expression then;
	Late<bool> is_own;
};

struct Seq {
	Expression first;
	Expression then;
};

struct Case {
	Expression cond;
	Expression then;
};
