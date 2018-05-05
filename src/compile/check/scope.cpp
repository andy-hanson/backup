#include "./scope.h"

Option<Ref<const SpecDeclaration>> find_spec(const StringSlice& name, CheckCtx& ctx, const SpecsTable& specs_table) {
	Option<Ref<const SpecDeclaration>> res = copy_inner(specs_table.get(name));
	for (Ref<const Module> m : ctx.imports) {
		Option<Ref<const SpecDeclaration>> s = copy_inner(m->specs_table.get(name));
		if (s.has() && s.get()->is_public) {
			if (res.has()) throw "todo"; else res = s;
		}
	}
	return res;
}

Option<Ref<const StructDeclaration>> find_struct(const StringSlice& name, CheckCtx& ctx, const StructsTable& structs_table) {
	Option<Ref<const StructDeclaration>> res = copy_inner(structs_table.get(name));
	for (Ref<const Module> m : ctx.imports) {
		Option<Ref<const StructDeclaration>> op_2 = copy_inner(m->structs_table.get(name));
		if (op_2.has() && op_2.get()->is_public) {
			if (res.has()) throw "todo"; else res = op_2;
		}
	}
	return res;
}
