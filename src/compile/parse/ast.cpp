#include "./ast.h"

void FunBodyAst::operator=(const FunBodyAst& other) {
	_kind = other._kind;
	switch (_kind) {
		case Kind::CppSource:
			_data.cpp_source = other._data.cpp_source;
			break;
		case Kind::Expression:
			_data.expression = other._data.expression;
			break;
	}
}
