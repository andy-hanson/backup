#pragma once

#include "parse_diag.h"
#include "../emit/Writer.h"

struct WrongNumber {
	size_t expected;
	size_t actual;
};

class Diag {
public:
	enum Kind {
		Parse,

		// Top-level diags
		SpecNameNotFound,
		DuplicateDeclaration,
		SpecialTypeShouldNotHaveTypeParameters,
		WrongNumberTypeArguments,
		TypeParameterShadowsStruct,
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
			case Kind::SpecNameNotFound:
			case Kind::DuplicateDeclaration:
			case Kind::SpecialTypeShouldNotHaveTypeParameters:
			case Kind::UnrecognizedParameterOrLocal:
			case Kind::CantCreateNonStruct:
			case Kind::UnnecessaryTypeAnnotate:
			case Kind::TypeParameterShadowsStruct:
			case Kind::TypeParameterShadowsSpecTypeParameter:
			case Kind::TypeParameterShadowsPrevious:
			case Kind::LocalShadowsFun:
			case Kind::LocalShadowsSpecSig:
			case Kind::LocalShadowsParameter:
			case Kind::LocalShadowsLocal:
				break;
			case Kind::WrongNumberTypeArguments:
			case Kind::WrongNumberNewStructArguments:
				data.wrong_number = other.data.wrong_number;
				break;
		}
	}

	Diag(ParseDiag p) : _kind(Kind::Parse) { data.parse_diag = p; }
	Diag(Kind kind) : _kind(kind) {
		switch (kind) {
			case Kind::SpecNameNotFound:
			case Kind::DuplicateDeclaration:
			case Kind::SpecialTypeShouldNotHaveTypeParameters:
			case Kind::UnrecognizedParameterOrLocal:
			case Kind::CantCreateNonStruct:
			case Kind::UnnecessaryTypeAnnotate:
			case Kind::TypeParameterShadowsStruct:
			case Kind::TypeParameterShadowsSpecTypeParameter:
			case Kind::TypeParameterShadowsPrevious:
			case Kind::LocalShadowsFun:
			case Kind::LocalShadowsSpecSig:
			case Kind::LocalShadowsParameter:
			case Kind::LocalShadowsLocal:
				break;
			case Kind::Parse:
			case Kind::WrongNumberTypeArguments:
			case Kind::WrongNumberNewStructArguments:
				assert(false);
		}
	}
	Diag(Kind kind, WrongNumber wrong_number) {
		assert(kind == Kind::WrongNumberTypeArguments || kind == Kind::WrongNumberNewStructArguments);
		data.wrong_number = wrong_number;
	}

	void write(Writer& out, const StringSlice& slice) {
		switch (_kind) {
			case Kind::Parse:
				out << data.parse_diag;
				break;

			case Kind::SpecNameNotFound:
				out << "Could not find a spec named '" << slice << "'";
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
			case Kind::TypeParameterShadowsStruct:
				out << "Type parameter shadows a struct";
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
		}
	}
};

struct Diagnostic {
	SourceRange range;
	Diag diag;

	Diagnostic(SourceRange _range, Diag _diag) : range(_range), diag(_diag) {}
	Diagnostic(ParseDiagnostic p) : range(p.range), diag(p.diag) {}

	void write(Writer& out, const StringSlice& source) {
		out << '[' << range.begin << ':' << range.end << "]: ";
		diag.write(out,  StringSlice::from_range(source, range));
	}
};
