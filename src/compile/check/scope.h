#pragma once

#include "../model/model.h"
#include "./check_common.h"

Option<ref<const SpecDeclaration>> find_spec(const StringSlice& name, CheckCtx& ctx, const SpecsTable& specs_table);
Option<ref<const StructDeclaration>> find_struct(const StringSlice& name, CheckCtx& ctx, const StructsTable& structs_table);

template <typename /*CalledDeclaration => void*/ Cb>
void each_fun_with_name(const ExprContext& ctx, const StringSlice& name, Cb cb) {
	for (ref<const FunDeclaration> f : ctx.funs_table.all_with_key(name))
		cb(f);
	for (ref<const Module> m : ctx.check_ctx.imports)
		for (ref<const FunDeclaration> f : m->funs_table.all_with_key(name))
			if (f->is_public)
				cb(f);
}
