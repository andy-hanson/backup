#pragma once

#include "../../util/Arena.h"
#include "../../util/ArenaString.h"
#include "../../util/assert.h"
#include "../../util/StringSlice.h"
#include "./type_ast.h"

class ExprAst;

struct CallAst {
	StringSlice fun_name;
	Slice<TypeAst> type_arguments;
	Slice<ExprAst> arguments;
};

struct LiteralAst {
	StringSlice literal;
	Slice<TypeAst> type_arguments;
	Slice<ExprAst> arguments;
};

struct StructCreateAst {
	StringSlice struct_name;
	Slice<TypeAst> type_arguments;
	Slice<ExprAst> arguments;
};

struct TypeAnnotateAst;

struct LetAst {
	StringSlice name;
	Ref<ExprAst> init;
	Ref<ExprAst> then;
};
struct SeqAst {
	SourceRange range;
	Ref<ExprAst> first;
	Ref<ExprAst> then;
};
struct CaseAst;
struct WhenAst {
	SourceRange range;
	Slice<CaseAst> cases;
	Ref<ExprAst> elze;
};

struct AssertAst {
	SourceRange range;
	Ref<ExprAst> asserted;
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
		Ref<TypeAnnotateAst> type_annotate;
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
	void operator=(const ExprAst& other);

	inline explicit ExprAst(StringSlice identifier) : _kind(Kind::Identifier) {
		data.identifier = identifier;
	}
	inline explicit ExprAst(LiteralAst literal) : _kind(Kind::Literal) {
		data.literal = literal;
	}
	inline explicit ExprAst(ArenaString no_call_literal) : _kind(Kind::NoCallLiteral) {
		data.no_call_literal = no_call_literal;
	}
	inline explicit ExprAst(CallAst call) : _kind(Kind::Call) {
		data.call = call;
	}
	inline explicit ExprAst(StructCreateAst struct_create) : _kind(Kind::StructCreate) {
		data.struct_create = struct_create;
	}
	inline explicit ExprAst(Ref<TypeAnnotateAst> type_annotate) : _kind(Kind::TypeAnnotate) {
		data.type_annotate = type_annotate;
	}
	inline explicit ExprAst(LetAst let) : _kind(Kind::Let) {
		data.let = let;
	}
	inline explicit ExprAst(SeqAst seq) : _kind(Kind::Seq) {
		data.seq = seq;
	}
	inline explicit ExprAst(WhenAst when) : _kind(Kind::When) {
		data.when = when;
	}
	inline explicit ExprAst(AssertAst assert) : _kind(Kind::Assert) {
		data.assert = assert;
	}
	inline ExprAst(SourceRange range, Kind kind) : _kind(Kind::Pass) {
		data.pass = range;
		assert(kind == Kind::Pass);
	}

	inline const StringSlice& identifier() const {
		assert(_kind == Kind::Identifier);
		return data.identifier;
	}
	inline const LiteralAst& literal() const {
		assert(_kind == Kind::Literal);
		return data.literal;
	}
	inline const ArenaString& no_call_literal() const {
		assert(_kind == Kind::NoCallLiteral);
		return data.no_call_literal;
	}
	inline const CallAst& call() const {
		assert(_kind == Kind::Call);
		return data.call;
	}
	inline const StructCreateAst& struct_create() const {
		assert(_kind == Kind::StructCreate);
		return data.struct_create;
	}
	inline const TypeAnnotateAst& type_annotate() const {
		assert(_kind == Kind::TypeAnnotate);
		return data.type_annotate;
	}
	inline const LetAst& let() const {
		assert(_kind == Kind::Let);
		return data.let;
	}
	inline const SeqAst& seq() const {
		assert(_kind == Kind::Seq);
		return data.seq;
	}
	inline const WhenAst& when() const {
		assert(_kind == Kind::When);
		return data.when;
	}
	inline const AssertAst& assert_ast() const {
		assert(_kind == Kind::Assert);
		return data.assert;
	}
	inline const SourceRange& pass() const {
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
