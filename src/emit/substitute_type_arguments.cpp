#include "./substitute_type_arguments.h"

namespace {
	// Recursively replaces every type parameter with a corresponding type argument.
	InstStruct substitute_type_arguments(const TypeParameter& t, const Slice<TypeParameter>& type_parameters, const Slice<InstStruct>& type_arguments) {
		assert(&type_parameters[t.index] == &t);
		return type_arguments[t.index];
	}

	// Recursively replaces every type parameter with a corresponding type argument.
	InstStruct substitute_type_arguments(const StoredType& t, const Slice<TypeParameter>& params, const Slice<InstStruct>& args, Arena& arena) {
		if (t.is_type_parameter())
			return substitute_type_arguments(t.param(), params, args);
		else {
			const InstStruct& i = t.inst_struct();
			if (!some(i.type_arguments, [](const Type& ta) { return ta.stored_type().is_type_parameter(); }))
				return i;
			//TODO: noborrow is wrong. We should distinguish Vec<Int> and vec<Int *>.
			return InstStruct {
				i.strukt,
				map<Type>()(arena, i.type_arguments, [&](const Type& ta) {
					return Type::noborrow(StoredType { substitute_type_arguments(ta.stored_type(), params, args, arena) });
				})
			};
		}
	}
}

InstStruct substitute_type_arguments(const StoredType& type, const ConcreteFun& fun, Arena& arena) {
	return substitute_type_arguments(type, fun.fun_declaration->signature.type_parameters, fun.type_arguments, arena);
}
