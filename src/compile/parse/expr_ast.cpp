#include "./expr_ast.h"

void ExprAst::operator=(const ExprAst& other) {

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
