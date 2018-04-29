#include "./expr.h"

void CalledDeclaration::operator=(const CalledDeclaration& other) {
	_kind = other._kind;
	switch (_kind) {
		case Kind::Fun: data.fun_decl = other.data.fun_decl; break;
		case Kind::Spec: data.spec_use_sig = other.data.spec_use_sig; break;
	}
}

const FunSignature& CalledDeclaration::sig() const {
	switch (_kind) {
		case Kind::Fun:
			return fun()->signature;
		case Kind::Spec:
			return spec().signature;
	}
}

void Expression::operator=(const Expression& e) {
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
