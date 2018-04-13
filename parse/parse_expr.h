#pragma once

#include "Lexer.h"

class ExprAst;

struct TypeAst {
	Effect effect;
	StringSlice type_name;
	DynArray<TypeAst> type_arguments;
};

struct CallAst {
	StringSlice fun_name;
	DynArray<TypeAst> type_arguments;
	DynArray<ExprAst> arguments;
};

struct LiteralAst {
	ArenaString literal;
	DynArray<TypeAst> type_arguments;
	DynArray<ExprAst> arguments;
};

struct StructCreateAst {
	StringSlice struct_name;
	DynArray<TypeAst> type_arguments;
	DynArray<ExprAst> arguments;
};

struct TypeAnnotateAst;

struct LetAst {
	StringSlice name;
	ref<ExprAst> init;
	ref<ExprAst> then;
};
struct CaseAst;
struct WhenAst {
	DynArray<CaseAst> cases;
	ref<ExprAst> elze;
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
		When,
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
		WhenAst when;
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
			case Kind::When:
				data.when = other.data.when;
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
	ExprAst(WhenAst when) : _kind(Kind::When) {
		data.when = when;
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
	const WhenAst& when() const {
		assert(_kind == Kind::When);
		return data.when;
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

ExprAst parse_body_ast(Lexer& lexer, Arena& expr_ast_arena, Arena& literals_arena);
