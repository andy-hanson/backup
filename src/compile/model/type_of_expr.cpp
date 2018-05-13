#include "./type_of_expr.h"

Type type_of_expr(Expression e, const BuiltinTypes& builtin_types) {
	switch (e.kind()) {
		case Expression::Kind::Nil:
		case Expression::Kind::Bogus:
			unreachable();

		case Expression::Kind::ParameterReference:
			return e.parameter_reference()->type;

		case Expression::Kind::LocalReference:
			return e.local_reference()->type;

		case Expression::Kind::StructFieldAccess:
			return e.struct_field_access().accessed_field_type;

		case Expression::Kind::Let:
			return type_of_expr(e.let().then, builtin_types);

		case Expression::Kind::Seq:
			return type_of_expr(e.seq().then, builtin_types);

		case Expression::Kind::Call:
			return e.call().concrete_return_type;

		case Expression::Kind::StructCreate:
			return Type::noborrow(StoredType { e.struct_create().inst_struct });

		case Expression::Kind::StringLiteral:
			return builtin_types.string_type.get();

		case Expression::Kind::When:
			// Should all have the same type, so...
			return e.when().type;

		case Expression::Kind::Assert:
		case Expression::Kind::Pass:
			return builtin_types.void_type.get();
	}
}
