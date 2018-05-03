#pragma once

#include "../model/model.h"
#include "./check_common.h"

Option<ref<const SpecDeclaration>> find_spec(const StringSlice& name, CheckCtx& ctx, const SpecsTable& specs_table);
Option<ref<const StructDeclaration>> find_struct(const StringSlice& name, CheckCtx& ctx, const StructsTable& structs_table);

template <typename /*CalledDeclaration => void*/ Cb>
void each_fun_with_name(const ExprContext& ctx, const StringSlice& name, Cb cb) {
	ctx.funs_table.each_with_key(name, [&](ref<const FunDeclaration> f) { cb(CalledDeclaration { f }); });
	for (ref<const Module> m : ctx.check_ctx.imports)
		m->funs_table.each_with_key(name, [&](ref<const FunDeclaration> f) {
			if (f->is_public) cb(CalledDeclaration { f });
		});
}
