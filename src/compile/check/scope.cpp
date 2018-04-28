#include "./scope.h"

Option<ref<const SpecDeclaration>> find_spec(const StringSlice& name, CheckCtx& ctx, const SpecsTable& specs_table) {
	Option<ref<const SpecDeclaration>> res = specs_table.get(name).copy_inner();
	for (ref<const Module> m : ctx.imports) {
		Option<ref<const SpecDeclaration>> s = m->specs_table.get(name).copy_inner();
		if (s.has() && s.get()->is_public) {
			if (res.has()) throw "todo"; else res = s;
		}
	}
	return res;
}

Option<ref<const StructDeclaration>> find_struct(const StringSlice& name, CheckCtx& ctx, const StructsTable& structs_table) {
	Option<ref<const StructDeclaration>> res = structs_table.get(name).copy_inner();
	for (ref<const Module> m : ctx.imports) {
		Option<ref<const StructDeclaration>> op_2 = m->structs_table.get(name).copy_inner();
		if (op_2.has() && op_2.get()->is_public) {
			if (res.has()) throw "todo"; else res = op_2;
		}
	}
	return res;
}
