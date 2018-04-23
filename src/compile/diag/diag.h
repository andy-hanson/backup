#pragma once

#include "parse_diag.h"
#include "../../util/Writer.h"

struct WrongNumber {
	size_t expected;
	size_t actual;
};

class Diag {
public:
	enum Kind {
		Parse,

		// Top-level diags
		StructNameNotFound,
		TypeParameterNameNotFound,
		SpecNameNotFound,
		DuplicateDeclaration,
		SpecialTypeShouldNotHaveTypeParameters,
		WrongNumberTypeArguments,
		TypeParameterShadowsSpecTypeParameter,
		TypeParameterShadowsPrevious,

		// Check-expr diags
		UnrecognizedParameterOrLocal,
		CantCreateNonStruct,
		WrongNumberNewStructArguments,
		UnnecessaryTypeAnnotate,
		LocalShadowsFun,
		LocalShadowsSpecSig,
		LocalShadowsParameter,
		LocalShadowsLocal,
		MissingVoidType,
		MissingBoolType,
		MissingStringType,
	};

private:
	Kind _kind;
	union Data {
		ParseDiag parse_diag;
		WrongNumber wrong_number;

		Data() {}
		~Data() {}
	};
	Data data;

public:
	Diag(const Diag& other) : _kind(other._kind) {
		switch (_kind) {
			case Kind::Parse:
				data.parse_diag = other.data.parse_diag;
				break;
			case Kind::WrongNumberTypeArguments:
			case Kind::WrongNumberNewStructArguments:
				data.wrong_number = other.data.wrong_number;
				break;
			case Kind::SpecNameNotFound:
			case Kind::StructNameNotFound:
			case Kind::TypeParameterNameNotFound:
			case Kind::DuplicateDeclaration:
			case Kind::SpecialTypeShouldNotHaveTypeParameters:
			case Kind::UnrecognizedParameterOrLocal:
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

	Diag(ParseDiag p) : _kind(Kind::Parse) { data.parse_diag = p; }
	Diag(Kind kind) : _kind(kind) {
		switch (_kind) {
			case Kind::Parse:
			case Kind::WrongNumberTypeArguments:
			case Kind::WrongNumberNewStructArguments:
				assert(false);

			case Kind::SpecNameNotFound:
			case Kind::StructNameNotFound:
			case Kind::TypeParameterNameNotFound:
			case Kind::DuplicateDeclaration:
			case Kind::SpecialTypeShouldNotHaveTypeParameters:
			case Kind::UnrecognizedParameterOrLocal:
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
	Diag(Kind kind, WrongNumber wrong_number) {
		assert(kind == Kind::WrongNumberTypeArguments || kind == Kind::WrongNumberNewStructArguments);
		data.wrong_number = wrong_number;
	}

	void write(Writer& out, const StringSlice& slice) const {
		switch (_kind) {
			case Kind::Parse:
				out << data.parse_diag;
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

			case Kind::UnrecognizedParameterOrLocal:
				out << "Could not find a parameter or local variable named '" << slice << "'";
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
};

struct Diagnostic {
	SourceRange range;
	Diag diag;

	Diagnostic(SourceRange _range, Diag _diag) : range(_range), diag(_diag) {}
	Diagnostic(ParseDiagnostic p) : range(p.range), diag(p.diag) {}

	void write(Writer& out, const StringSlice& source) const {
		out << '[' << range.begin << ':' << range.end << "]: ";
		diag.write(out,  StringSlice::from_range(source, range));
	}
};
