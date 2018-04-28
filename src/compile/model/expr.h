#pragma once

#include "../../util/Alloc.h"
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

	const FunSignature& sig() const {
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
	Arr<Type> type_arguments;
	// These will be in order of the signatures from the specs of the function we're calling.
	// For each spec, there is an array of a Called matching each signature in that spec.
	// Note: if there is a generic spec signature, it should be matched by a generic function. And a non-generic spec signature can't be matched by a generic function.
	// So this uses CalledDeclaration and not Called.
	Arr<Arr<CalledDeclaration>> spec_impls;
};

struct Call {
	Called called;
	Arr<Expression> arguments;
};

struct StructCreate {
	InstStruct inst_struct; // Effect is Io
	Arr<Expression> arguments;
};

struct Let;
struct Seq;

struct Case;
struct When {
	Arr<Case> cases;
	ref<Expression> elze;

	When(Arr<Case> _cases, ref<Expression> _elze) : cases(_cases), elze(_elze) {
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
		ref<const Parameter> parameter_reference;
		ref<const Let> local_reference;
		StructFieldAccess struct_field_access;
		ref<Let> let;
		ref<Seq> seq;
		Call call;
		StructCreate struct_create;
		ArenaString string_literal;
		When when;
		ref<Expression> asserted;

		Data() {} // uninitialized
		~Data() {} // Nothing in here should need to be deleted
	};

	Kind _kind;
	Data data;

public:
	Kind kind() const { return _kind; }

	Expression() : _kind(Kind::Nil) {}

	static Expression bogus() {
		Expression e;
		e._kind = Kind::Bogus;
		return e;
	}

	Expression(const Expression& e) {
		_kind = Kind::Nil; // operator= asserts this
		*this = e;
	}

	Expression(ref<const Parameter> p) : _kind(Kind::ParameterReference) {
		data.parameter_reference = p;
	}

	Expression(ref<const Let> l, Kind kind) : _kind(kind) {
		assert(kind == Kind::LocalReference);
		data.local_reference = l;
	}

	Expression(ref<Let> l, Kind kind) : _kind(kind) {
		assert(kind == Kind::Let);
		data.let = l;
	}

	Expression(ref<Seq> seq) : _kind(Kind::Seq) {
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

	Expression(ref<Expression> asserted, Kind kind) : _kind(kind) {
		assert(kind == Kind::Assert);
		data.asserted = asserted;
	}

	Expression(Kind kind) : _kind(Kind::Pass) {
		assert(kind == Kind::Pass);
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
	StructFieldAccess& struct_field_access() {
		assert(_kind == Kind::StructFieldAccess);
		return data.struct_field_access;
	}
	const StructFieldAccess& struct_field_access() const {
		assert(_kind == Kind::StructFieldAccess);
		return data.struct_field_access;
	}
	Let& let() {
		assert(_kind == Kind::Let);
		return *data.let;
	}
	const Let& let() const {
		assert(_kind == Kind::Let);
		return *data.let;
	}
	Seq& seq() {
		assert(_kind == Kind::Seq);
		return *data.seq;
	}
	const Seq& seq() const {
		assert(_kind == Kind::Seq);
		return *data.seq;
	}
	Call& call() {
		assert(_kind == Kind::Call);
		return data.call;
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
	Expression& asserted() {
		assert(_kind == Kind::Assert);
		return data.asserted;
	}
	const Expression& asserted() const {
		assert(_kind == Kind::Assert);
		return data.asserted;
	}

	void operator=(const Expression& e) {
		_kind = e._kind;
		switch (_kind) {
			case Kind::Nil:
			case Kind::Bogus:
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
			case Kind::Assert:
				data.asserted = e.data.asserted;
				break;
			case Kind::Pass:
				break;
		}
	}
};

struct Let {
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
