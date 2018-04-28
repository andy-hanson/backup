#pragma once

#include <cassert>

#include "../../util/Alloc.h"
#include "../../util/StringSlice.h"
#include "./type_ast.h"

class ExprAst;

struct CallAst {
	StringSlice fun_name;
	Arr<TypeAst> type_arguments;
	Arr<ExprAst> arguments;
};

struct LiteralAst {
	StringSlice literal;
	Arr<TypeAst> type_arguments;
	Arr<ExprAst> arguments;
};

struct StructCreateAst {
	StringSlice struct_name;
	Arr<TypeAst> type_arguments;
	Arr<ExprAst> arguments;
};

struct TypeAnnotateAst;

struct LetAst {
	StringSlice name;
	ref<ExprAst> init;
	ref<ExprAst> then;
};
struct SeqAst {
	SourceRange range;
	ref<ExprAst> first;
	ref<ExprAst> then;
};
struct CaseAst;
struct WhenAst {
	SourceRange range;
	Arr<CaseAst> cases;
	ref<ExprAst> elze;
};

struct AssertAst {
	SourceRange range;
	ref<ExprAst> asserted;
};

class ExprAst {
public:
	enum class Kind {
		Identifier,
		Literal,
		NoCallLiteral,
		Call,
		StructCreate,
		TypeAnnotate,
		Let,
		Seq,
		When,
		Assert,
		Pass,
	};

private:
	union Data {
		StringSlice identifier;
		LiteralAst literal;
		ArenaString no_call_literal;
		CallAst call;
		StructCreateAst struct_create;
		ref<TypeAnnotateAst> type_annotate;
		LetAst let;
		SeqAst seq;
		WhenAst when;
		AssertAst assert;
		SourceRange pass;
		Data() {} // uninitialized
		~Data() {} // It's in an arena, don't need a destructor
	};
	Kind _kind;
	Data data;

public:
	Kind kind() const { return _kind; }

	ExprAst(const ExprAst& other) {
		*this = other;
	}
	void operator=(const ExprAst& other) {
		_kind = other._kind;
		switch (_kind) {
			case Kind::Identifier:
				data.identifier = other.data.identifier;
				break;
			case Kind::Literal:
				data.literal = other.data.literal;
				break;
			case Kind::NoCallLiteral:
				data.no_call_literal = other.data.no_call_literal;
				break;
			case Kind::Call:
				data.call = other.data.call;
				break;
			case Kind::StructCreate:
				data.struct_create = other.data.struct_create;
				break;
			case Kind::TypeAnnotate:
				data.type_annotate = other.data.type_annotate;
				break;
			case Kind::Let:
				data.let = other.data.let;
				break;
			case Kind::Seq:
				data.seq = other.data.seq;
				break;
			case Kind::When:
				data.when = other.data.when;
				break;
			case Kind::Assert:
				data.assert = other.data.assert;
				break;
			case Kind::Pass:
				data.pass = other.data.pass;
				break;
		}
	}

	ExprAst(StringSlice identifier) : _kind(Kind::Identifier) {
		data.identifier = identifier;
	}
	ExprAst(LiteralAst literal) : _kind(Kind::Literal) {
		data.literal = literal;
	}
	ExprAst(ArenaString no_call_literal) : _kind(Kind::NoCallLiteral) {
		data.no_call_literal = no_call_literal;
	}
	ExprAst(CallAst call) : _kind(Kind::Call) {
		data.call = call;
	}
	ExprAst(StructCreateAst struct_create) : _kind(Kind::StructCreate) {
		data.struct_create = struct_create;
	}
	ExprAst(ref<TypeAnnotateAst> type_annotate) : _kind(Kind::TypeAnnotate) {
		data.type_annotate = type_annotate;
	}
	ExprAst(LetAst let) : _kind(Kind::Let) {
		data.let = let;
	}
	ExprAst(SeqAst seq) : _kind(Kind::Seq) {
		data.seq = seq;
	}
	ExprAst(WhenAst when) : _kind(Kind::When) {
		data.when = when;
	}
	ExprAst(AssertAst assert) : _kind(Kind::Assert) {
		data.assert = assert;
	}
	ExprAst(SourceRange range, Kind kind) : _kind(Kind::Pass) {
		data.pass = range;
		assert(kind == Kind::Pass);
	}

	const StringSlice& identifier() const {
		assert(_kind == Kind::Identifier);
		return data.identifier;
	}
	const LiteralAst& literal() const {
		assert(_kind == Kind::Literal);
		return data.literal;
	}
	const ArenaString& no_call_literal() const {
		assert(_kind == Kind::NoCallLiteral);
		return data.no_call_literal;
	}
	const CallAst& call() const {
		assert(_kind == Kind::Call);
		return data.call;
	}
	const StructCreateAst& struct_create() const {
		assert(_kind == Kind::StructCreate);
		return data.struct_create;
	}
	const TypeAnnotateAst& type_annotate() const {
		assert(_kind == Kind::TypeAnnotate);
		return data.type_annotate;
	}
	const LetAst& let() const {
		assert(_kind == Kind::Let);
		return data.let;
	}
	const SeqAst& seq() const {
		assert(_kind == Kind::Seq);
		return data.seq;
	}
	const WhenAst& when() const {
		assert(_kind == Kind::When);
		return data.when;
	}
	const AssertAst& assert_ast() const {
		assert(_kind == Kind::Assert);
		return data.assert;
	}
	const SourceRange& pass() const {
		assert(_kind == Kind::Pass);
		return data.pass;
	}
};

struct CaseAst {
	ExprAst condition;
	ExprAst then;
};

struct TypeAnnotateAst {
	TypeAst type;
	ExprAst expression;
};
