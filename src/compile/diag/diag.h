#pragma once

#include "parse_diag.h"
#include "../../util/Writer.h"
#include "../model/model.h"
#include "./LineAndColumnGetter.h"

struct WrongNumber {
	uint expected;
	uint actual;
};

class Diag {
public:
	enum Kind {
		Parse,

		CircularImport,

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
		CantCreateNonStruct,
		WrongNumberNewStructArguments,
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
	Diag(const Diag& other);
	void operator=(const Diag& other);
	Diag(ParseDiag p);
	Diag(Kind kind);
	Diag(Kind kind, WrongNumber wrong_number);

	void write(Writer& out, const StringSlice& slice) const;
};

struct Diagnostic {
	Path path;
	SourceRange range;
	Diag diag;

	Diagnostic(Path _path, SourceRange _range, Diag _diag) : path(_path), range(_range), diag(_diag) {}
	Diagnostic(Path _path, ParseDiagnostic p) : path(_path), range(p.range), diag(p.diag) {}

	void write(Writer& out, const StringSlice& source, const LineAndColumnGetter& lc) const;
};
