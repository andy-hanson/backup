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

struct StructCreateAst {
	StringSlice struct_name;
	DynArray<TypeAst> type_arguments;
	DynArray<ExprAst> arguments;
};

struct LetAst {
	StringSlice name;
	ref<ExprAst> init;
	ref<ExprAst> then;
};
struct CaseAst;
struct WhenAst {
	List<CaseAst> cases;
	ref<ExprAst> elze;
};

class ExprAst {
public:
	enum class Kind {
		Identifier,
		Call,
		StructCreate,
		Let,
		UintLiteral,
		When,
	};

private:
	union Data {
		StringSlice identifier;
		CallAst call;
		StructCreateAst struct_create;
		LetAst let;
		uint64_t uint_literal;
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
			case Kind::Call:
				data.call = other.data.call;
				break;
			case Kind::StructCreate:
				data.struct_create = other.data.struct_create;
				break;
			case Kind::Let:
				data.let = other.data.let;
				break;
			case Kind::UintLiteral:
				data.uint_literal = other.data.uint_literal;
				break;
			case Kind::When:
				data.when = other.data.when;
				break;
		}
	}

	ExprAst(StringSlice identifier) : _kind(Kind::Identifier) {
		data.identifier = identifier;
	}
	ExprAst(CallAst call) : _kind(Kind::Call) {
		data.call = call;
	}
	ExprAst(StructCreateAst struct_create) : _kind(Kind::StructCreate) {
		data.struct_create = struct_create;
	}
	ExprAst(LetAst let) : _kind(Kind::Let) {
		data.let = let;
	}
	ExprAst(uint64_t uint_literal) : _kind(Kind::UintLiteral) {
		data.uint_literal = uint_literal;
	}
	ExprAst(WhenAst when) : _kind(Kind::When) {
		data.when = when;
	}

	const StringSlice& identifier() const {
		assert(_kind == Kind::Identifier);
		return data.identifier;
	}
	const CallAst& call() const {
		assert(_kind == Kind::Call);
		return data.call;
	}
	const StructCreateAst& struct_create() const {
		assert(_kind == Kind::StructCreate);
		return data.struct_create;
	}
	const LetAst& let() const {
		assert(_kind == Kind::Let);
		return data.let;
	}
	uint64_t uint_literal() const {
		assert(_kind == Kind::UintLiteral);
		return data.uint_literal;
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

ExprAst parse_body_ast(Lexer& lexer, Arena& arena);
