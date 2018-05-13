#include "./convert_type.h"

#include "../../util/store/collection_util.h" // find_in_either

namespace {
	Option<Ref<const StructDeclaration>> find_struct(const StringSlice& name, CheckCtx& ctx, const StructsTable& structs_table) {
		Option<Ref<const StructDeclaration>> res = copy_inner(structs_table.get(name));
		for (Ref<const Module> m : ctx.imports) {
			Option<Ref<const StructDeclaration>> s = copy_inner(m->structs_table.get(name));
			if (s.has() && s.get()->is_public) {
				if (res.has())
					todo();
				else res =
					 s;
			}
		}
		return res;
	}

	StoredType type_from_type_parameter(const StringSlice& name, CheckCtx& ctx, const TypeParametersScope& type_parameters_scope) {
		Option<Ref<const TypeParameter>> tp = find_in_either(type_parameters_scope.outer, type_parameters_scope.inner, [&](const TypeParameter& t) { return t.name == name; });
		if (!tp.has()) {
			ctx.diag(name, Diag::Kind::TypeParameterNameNotFound);
			return StoredType::bogus();
		}
		return StoredType { tp.get() };
	}

	StoredType type_from_struct(
		const StringSlice& name, const Slice<TypeAst>& type_arguments, CheckCtx& ctx, const StructsTable& structs_table, Option<const Slice<Parameter>&> parameters, const TypeParametersScope& type_parameters_scope) {
		Option<Ref<const StructDeclaration>> op_strukt = find_struct(name, ctx, structs_table);
		if (!op_strukt.has()) {
			ctx.diag(name, Diag::Kind::StructNameNotFound);
			return StoredType::bogus();
		}

		Ref<const StructDeclaration> strukt = op_strukt.get();
		if (type_arguments.size() != strukt->type_parameters.size()) todo();
		return StoredType { InstStruct { strukt, type_arguments_from_asts(type_arguments, ctx, structs_table, parameters, type_parameters_scope) } };
	}

	StoredType stored_type_from_ast(const StoredTypeAst& ast, CheckCtx& ctx, const StructsTable& structs_table, Option<const Slice<Parameter>&> parameters, const TypeParametersScope& type_parameters_scope) {
		switch (ast.kind()) {
			case StoredTypeAst::Kind::TypeParameter:
				return type_from_type_parameter(ast.name(), ctx, type_parameters_scope);
			case StoredTypeAst::Kind::InstStruct:
				return type_from_struct(ast.name(), ast.type_arguments(), ctx, structs_table, parameters, type_parameters_scope);
		}
	}

	Lifetime lifetime_from_constraint(const LifetimeConstraintAst& ast, Option<const Slice<Parameter>&> parameters) {
		switch (ast.kind) {
			case LifetimeConstraintAst::Kind::ParameterName: {
				if (!parameters.has())
					todo(); // Diagnostic: Can't use parameter lifetime inside a struct
				Option<Ref<const Parameter>> param = find(parameters.get(), [&](const Parameter& p) { return p.name == ast.name; });
				if (!param.has())
					todo(); // Diagnostic: no such parameter
				return Lifetime::of_parameter(param.get()->index);
			}
			case LifetimeConstraintAst::Kind::LifetimeVariableName:
				todo();
		}
	}

	Lifetime lifetime_from_constraints(const Slice<LifetimeConstraintAst>& constraints, Option<const Slice<Parameter>&> parameters) {
		Lifetime::Builder b;
		for (const LifetimeConstraintAst& constraint : constraints)
			b.add(lifetime_from_constraint(constraint, parameters));
		return b.finish();
	}
	//	Lifetime::from_ast(ast.lifetime_constraints, [&](const LifetimeConstraintAst& l_ast) { return lifetime_from_constraint(l_ast, parameters); })
}

Type type_from_ast(const TypeAst& ast, CheckCtx& ctx, const StructsTable& structs_table, Option<const Slice<Parameter>&> parameters, const TypeParametersScope& type_parameters_scope) {
	return Type {
		stored_type_from_ast(ast.stored, ctx, structs_table, parameters, type_parameters_scope),
		lifetime_from_constraints(ast.lifetime_constraints, parameters)
	};
}

Slice<Type> type_arguments_from_asts(
	const Slice<TypeAst>& type_arguments, CheckCtx& al, const StructsTable& structs_table, Option<const Slice<Parameter>&> parameters, const TypeParametersScope& type_parameters_scope) {
	return map<Type>()(al.arena, type_arguments, [&](const TypeAst& t) { return type_from_ast(t, al, structs_table, parameters, type_parameters_scope); });
}
