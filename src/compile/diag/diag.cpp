#include "./diag.h"

namespace {
	Writer& operator<<(Writer& w, LineAndColumn lc) {
		return w << lc.line + 1 << ':' << lc.column + 1;
	}
}

Diag::Diag(ParseDiag p) : _kind(Kind::Parse) {
	data.parse_diag = p;
}

Diag::Diag(const Diag& other) { *this = other; }
void Diag::operator=(const Diag& other) {
	_kind = other._kind;
	switch (_kind) {
		case Kind::Parse:
			data.parse_diag = other.data.parse_diag;
			break;
		case Kind::WrongNumberTypeArguments:
		case Kind::WrongNumberNewStructArguments:
			data.wrong_number = other.data.wrong_number;
			break;
		case Kind::CircularImport:
		case Kind::SpecNameNotFound:
		case Kind::StructNameNotFound:
		case Kind::TypeParameterNameNotFound:
		case Kind::DuplicateDeclaration:
		case Kind::SpecialTypeShouldNotHaveTypeParameters:
		case Kind::CantCreateNonStruct:
		case Kind::UnnecessaryTypeAnnotate:
		case Kind::TypeParameterShadowsSpecTypeParameter:
		case Kind::TypeParameterShadowsPrevious:
		case Kind::LocalShadowsFun:
		case Kind::LocalShadowsSpecSig:
		case Kind::LocalShadowsParameter:
		case Kind::LocalShadowsLocal:
		case Kind::MissingBoolType:
		case Kind::MissingVoidType:
		case Kind::MissingStringType:
			break;
	}

}


Diag::Diag(Kind kind) : _kind(kind) {
	switch (_kind) {
		case Kind::Parse:
		case Kind::WrongNumberTypeArguments:
		case Kind::WrongNumberNewStructArguments:
			assert(false);

		case Kind::CircularImport:
		case Kind::SpecNameNotFound:
		case Kind::StructNameNotFound:
		case Kind::TypeParameterNameNotFound:
		case Kind::DuplicateDeclaration:
		case Kind::SpecialTypeShouldNotHaveTypeParameters:
		case Kind::CantCreateNonStruct:
		case Kind::UnnecessaryTypeAnnotate:
		case Kind::TypeParameterShadowsSpecTypeParameter:
		case Kind::TypeParameterShadowsPrevious:
		case Kind::LocalShadowsFun:
		case Kind::LocalShadowsSpecSig:
		case Kind::LocalShadowsParameter:
		case Kind::LocalShadowsLocal:
		case Kind::MissingBoolType:
		case Kind::MissingVoidType:
		case Kind::MissingStringType:
			break;
	}
}

Diag::Diag(Kind kind, WrongNumber wrong_number) {
	assert(kind == Kind::WrongNumberTypeArguments || kind == Kind::WrongNumberNewStructArguments);
	data.wrong_number = wrong_number;
}

void Diag::write(Writer& out, const StringSlice& slice) const {
	switch (_kind) {
		case Kind::Parse:
			data.parse_diag.write(out);
			break;
		case Kind::CircularImport:
			out << "Import of '" << slice << "' is circular";
			break;
		case Kind::SpecNameNotFound:
			out << "Could not find a spec named '" << slice << "'";
			break;
		case Kind::StructNameNotFound:
			out << "Could not find a struct named '" << slice << "'";
			break;
		case Kind::TypeParameterNameNotFound:
			out << "Could not find a type parameter named '" << slice << "'";
			break;
		case Kind::DuplicateDeclaration:
			out << "Duplicate declaration of '" << slice << "'";
			break;
		case Kind::SpecialTypeShouldNotHaveTypeParameters:
			out << "Special type " << slice << " should not have type parameters.";
			break;
		case Kind::WrongNumberTypeArguments:
			out << "Wrong number of type arguments. Expected " << data.wrong_number.expected << ", got " << data.wrong_number.actual << ".";
			break;
		case Kind::TypeParameterShadowsSpecTypeParameter:
			out << "Type parameter shadows one on the spec.";
			break;
		case Kind::TypeParameterShadowsPrevious:
			out << "Type parameter shadows a previous type parameter";
			break;

		case Kind::CantCreateNonStruct:
			out << "Can't create non-struct '" << slice << "'";
			break;
		case Kind::WrongNumberNewStructArguments:
			out << "Wrong number of arguments to struct. Expected " << data.wrong_number.expected << ", got " << data.wrong_number.actual << ".";
			break;
		case Kind::UnnecessaryTypeAnnotate:
			out << "Unnecessary type annotation -- an expected type already exists.";
			break;

		case Kind::LocalShadowsFun:
			out << "A function already has that name.";
			break;
		case Kind::LocalShadowsSpecSig:
			out << "A spec already has a signature with that name.";
			break;
		case Kind::LocalShadowsParameter:
			out << "A parameter already has that name.";
			break;
		case Kind::LocalShadowsLocal:
			out << "A previous local variable has the same name.";
			break;

		case Kind::MissingBoolType:
			out << "Must have a 'Bool' type to use a 'when' expression.";
			break;
		case Kind::MissingVoidType:
			out << "Must have a 'Void' type to use a statement.";
			break;
		case Kind::MissingStringType:
			out << "Must have a 'String' type to use literals.";
			break;
	}
}

void Diagnostic::write(Writer& out, const StringSlice& module_source, const LineAndColumnGetter& lc) const {
	out << this->path << ' ' << lc.line_and_column_at_pos(range.begin) << '-' << lc.line_and_column_at_pos(range.end) << ": ";
	diag.write(out, StringSlice::from_range(module_source, range));
}
