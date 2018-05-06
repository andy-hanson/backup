#include "./substitute_type_arguments.h"

namespace {
	// Recursively replaces every type parameter with a corresponding type argument.
	InstStruct substitute_type_arguments(const TypeParameter& t, const Slice<TypeParameter>& type_parameters, const Slice<InstStruct>& type_arguments) {
		assert(&type_parameters[t.index] == &t);
		return type_arguments[t.index];
	}

	// Recursively replaces every type parameter with a corresponding type argument.
	InstStruct substitute_type_arguments(const Type& t, const Slice<TypeParameter>& params, const Slice<InstStruct>& args, Arena& arena) {
		switch (t.kind()) {
			case Type::Kind::Nil:
			case Type::Kind::Bogus:
				// This should only be called from emit, and we don't emit if there were compile errors.
				unreachable();
			case Type::Kind::Param:
				return substitute_type_arguments(t.param(), params, args);
			case Type::Kind::InstStruct: {
				const InstStruct& i = t.inst_struct();
				return some(i.type_arguments, [](const Type& ta) { return ta.is_parameter(); })
					   ? InstStruct { i.strukt, map<Type>()(arena, i.type_arguments, [&](const Type& ta) { return Type{substitute_type_arguments(ta, params, args, arena)}; }) }
					   : i;
			}
		}
	}
}

InstStruct substitute_type_arguments(const Type& type_argument, const ConcreteFun& fun, Arena& arena) {
	return substitute_type_arguments(type_argument, fun.fun_declaration->signature.type_parameters, fun.type_arguments, arena);
}
