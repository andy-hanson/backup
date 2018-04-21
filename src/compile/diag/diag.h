#pragma once

#include "parse_diag.h"
#include "../emit/Writer.h"

class Diagnostic {
public:
	enum Kind {
		Parse,
	};

private:
	Kind _kind;
	SourceRange range;
	union Data {
		ParseDiag parse_diag;

		Data() {}
		~Data() {}
	};
	Data data;

public:
	Diagnostic(const Diagnostic& other) : _kind(other._kind) {
		switch (_kind) {
			case Kind::Parse:
				data.parse_diag = other.data.parse_diag;
		}
	}

	Diagnostic(ParseDiagnostic p) : _kind(Kind::Parse), range(p.range) {
		data.parse_diag = p.diag;
	}

	friend Writer& operator<<(Writer& out, const Diagnostic& d) {
		out << '[' << d.range.start << ':' << d.range.end << "]: ";
		switch (d._kind) {
			case Kind::Parse:
				return out << d.data.parse_diag;
		}
	}
};
