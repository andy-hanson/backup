#include "check.h"

#include "../../util/store/collection_util.h" // some
#include "./check_expr.h"
#include "./check_param_or_local_shadows_fun.h"
#include "./convert_type.h"

namespace {
	Option<Ref<const SpecDeclaration>> find_spec(const StringSlice& name, CheckCtx& ctx, const SpecsTable& specs_table) {
		Option<Ref<const SpecDeclaration>> res = copy_inner(specs_table.get(name));
		for (Ref<const Module> m : ctx.imports) {
			Option<Ref<const SpecDeclaration>> s = copy_inner(m->specs_table.get(name));
			if (s.has() && s.get()->is_public) {
				if (res.has())
					todo();
				else
					res = s;
			}
		}
		return res;
	}

	Slice<TypeParameter> check_type_parameters(const Slice<TypeParameterAst> asts, CheckCtx& al, const Slice<TypeParameter>& spec_type_parameters) {
		return map_with_prevs<TypeParameter>()(al.arena, asts, [&](const TypeParameterAst& ast, const Slice<TypeParameter>& prevs, uint index) {
			if (some(spec_type_parameters, [&](const TypeParameter& s) { return s.name == ast.name; }))
				al.diag(ast.name, Diag::Kind::TypeParameterShadowsSpecTypeParameter);
			for (const TypeParameter& prev : prevs)
				if (prev.name == ast.name)
					al.diag(ast.name, Diag::Kind::TypeParameterShadowsPrevious);
			assert(index == ast.index);
			return TypeParameter { al.range(ast.name), id(al, ast.name), index };
		});
	}


	Slice<Parameter> check_parameters(
		const Slice<ParameterAst>& asts, CheckCtx& al,
		const StructsTable& structs_table, const TypeParametersScope& type_parameters_scope, const FunsTable& funs_table, const Slice<SpecUse>& current_specs) {
		return map_with_prevs<Parameter>()(al.arena, asts, [&](const ParameterAst& ast, const Slice<Parameter>& prevs, uint index) -> Parameter {
			check_param_or_local_shadows_fun(al, ast.name, funs_table, current_specs);
			if (some(prevs, [&](const Parameter& prev) { return prev.name == ast.name; }))
				todo();
			Type type = type_from_ast(ast.type, al, structs_table, Option<const Slice<Parameter>&> { prevs }, type_parameters_scope);
			return Parameter { id(al, ast.name), type, index };
		});
	}

	Slice<SpecUse> check_spec_uses(const Slice<SpecUseAst>& asts, CheckCtx& ctx, const StructsTable& structs_table, const SpecsTable& specs_table, const TypeParametersScope& type_parameters_scope) {
		return map_op<SpecUse>()(ctx.arena, asts, [&](const SpecUseAst& ast) -> Option<SpecUse> {
			Option<Ref<const SpecDeclaration>> spec_op = find_spec(ast.spec, ctx, specs_table);
			if (!spec_op.has()) {
				ctx.diag(ast.spec, Diag::Kind::SpecNameNotFound);
				return {};
			};

			Ref<const SpecDeclaration> spec = spec_op.get();
			Slice<Type> args = type_arguments_from_asts(ast.type_arguments, ctx, structs_table, {}, type_parameters_scope);
			if (args.size() != spec->type_parameters.size()) {
				ctx.diag(ast.spec, { Diag::Kind::WrongNumberTypeArguments, { spec->type_parameters.size(), args.size() } });
				return {};
			}
			return Option<SpecUse>({ spec, args });
		});
	}

	FunSignature check_signature(
		const FunSignatureAst& ast, CheckCtx& ctx,
		const StructsTable& structs_table, const SpecsTable& specs_table, const FunsTable& funs_table,
		const Slice<TypeParameter>& spec_type_parameters, Identifier name
	) {
		Slice<TypeParameter> type_parameters = check_type_parameters(ast.type_parameters, ctx, spec_type_parameters);
		TypeParametersScope type_parameters_scope { spec_type_parameters, type_parameters };
		Slice<SpecUse> specs = check_spec_uses(ast.spec_uses, ctx, structs_table, specs_table, type_parameters_scope);
		Slice<Parameter> parameters = check_parameters(ast.parameters, ctx, structs_table, type_parameters_scope, funs_table, specs);
		Type return_type = type_from_ast(ast.return_type, ctx, structs_table, Option<const Slice<Parameter>&> { parameters }, type_parameters_scope);
		return { ctx.copy_str(ast.comment), type_parameters, return_type, name, parameters, specs };
	}

	Slice<StructField> check_struct_fields(const Slice<StructFieldAst>& asts, CheckCtx& ctx, const StructsTable& structs_table, const Slice<TypeParameter>& struct_type_parameters) {
		TypeParametersScope type_parameters_scope { {}, struct_type_parameters };
		return map<StructField>()(ctx.arena, asts, [&](const StructFieldAst& field) {
			return StructField { ctx.copy_str(field.comment), type_from_ast(field.type, ctx, structs_table, /*parameters*/ {}, type_parameters_scope), id(ctx, field.name) };
		});
	}

	void check_type_headers(const FileAst& file_ast, CheckCtx& ctx, Ref<Module> module) {
		if (!file_ast.includes.is_empty()) todo();

		module->specs_declaration_order = map<SpecDeclaration>()(ctx.arena, file_ast.specs, [&](const SpecDeclarationAst& ast) {
			return SpecDeclaration { module, ast.range, ctx.copy_str(ast.comment), ast.is_public, check_type_parameters(ast.type_parameters, ctx, {}), id(ctx, ast.name) };
		});
		module->specs_table = build_map<StringSlice, Ref<const SpecDeclaration>, StringSlice::hash>()(
			ctx.arena,
			module->specs_declaration_order,
			[](const SpecDeclaration& s) { return s.name; },
			[](const SpecDeclaration& s) { return Ref<const SpecDeclaration> { &s }; },
			[&](const SpecDeclaration& a __attribute__((unused)), const SpecDeclaration& b) {
				ctx.diag(b.name, Diag::Kind::DuplicateDeclaration);
			});

		module->structs_declaration_order = map<StructDeclaration>()(ctx.arena, file_ast.structs, [&](const StructDeclarationAst& ast) {
			return StructDeclaration { module, ast.range, ast.is_public, check_type_parameters(ast.type_parameters, ctx, {}), id(ctx, ast.name), ast.copy };
		});
		module->structs_table = build_map<StringSlice, Ref<const StructDeclaration>, StringSlice::hash>()(
			ctx.arena,
			module->structs_declaration_order,
			[](const StructDeclaration& s) { return s.name; },
			[](const StructDeclaration& s) { return Ref<const StructDeclaration> { &s }; },
			[&](const StructDeclaration& a __attribute__((unused)), const StructDeclaration& b) {
				ctx.diag(b.range, Diag::Kind::DuplicateDeclaration);
			});

		module->funs_declaration_order = map<FunDeclaration>()(ctx.arena, file_ast.funs, [&](const FunDeclarationAst& ast) {
			return FunDeclaration { module, ast.is_public, { id(ctx, ast.signature.name) }, {} };
		});
		module->funs_table = build_multi_map<StringSlice, Ref<const FunDeclaration>, StringSlice::hash>()(
			ctx.arena,
			module->funs_declaration_order,
			[](const FunDeclaration& f) { return f.name(); },
			[](const FunDeclaration& f) { return Ref<const FunDeclaration> { &f }; });
	}

	// Now that we've allocated every struct and spec, fill in every struct and spec, and fill in the header of every function.
	// Can't check function bodies at this point as it may call a future function -- we need to get the headers of every function first.
	void check_fun_headers_and_type_bodies(
		const FileAst& file_ast, CheckCtx& al,
		StructsDeclarationOrder& structs, SpecsDeclarationOrder& specs, FunsDeclarationOrder& funs,
		const StructsTable& structs_table, const SpecsTable& specs_table, FunsTable& funs_table
	) {
		zip(file_ast.specs, specs, [&](const SpecDeclarationAst& spec_ast, SpecDeclaration& spec) {
			spec.signatures = map<FunSignature>()(al.arena, spec_ast.signatures, [&](const FunSignatureAst& ast) {
				return check_signature(ast, al, structs_table, specs_table, funs_table, spec.type_parameters, id(al, ast.name));
			});
		});

		zip(file_ast.structs, structs, [&](const StructDeclarationAst& struct_ast, StructDeclaration& strukt) {
			const StructBodyAst& body_ast = struct_ast.body;
			strukt.body = body_ast.kind() == StructBodyAst::Kind::CppName
						   ? StructBody{ copy_string(al.arena, body_ast.cpp_name())}
						   : StructBody{check_struct_fields(body_ast.fields(), al, structs_table, strukt.type_parameters)};
		});

		zip(file_ast.funs, funs, [&](const FunDeclarationAst& fun_ast, FunDeclaration& fun) {
			// Name was allocated in the previous step.
			fun.signature = check_signature(fun_ast.signature, al, structs_table, specs_table, funs_table, {}, fun.signature.name);
		});
	}

	Expression check_function_body(
		const ExprAst& ast, CheckCtx& al, const FunsTable& funs_table, const StructsTable& structs_table, const FunDeclaration& fun, const BuiltinTypes& builtin_types) {
		Arena scratch_arena;
		ExprContext ctx { al, scratch_arena, funs_table, structs_table, &fun, {}, builtin_types };
		return check_and_expect_stored_type_and_lifetime(ast, ctx, fun.signature.return_type);
	}

	// Now that we have the bodies of every type and the headers of every function, we can fill in every function.
	void check_fun_bodies(const FileAst& file_ast, CheckCtx& ctx, const BuiltinTypes& builtin_types, FunsDeclarationOrder& funs, const StructsTable& structs_table, const FunsTable& funs_table) {
		zip(file_ast.funs, funs, [&](const FunDeclarationAst& ast, FunDeclaration& fun) {
			fun.body = ast.body.kind() == FunBodyAst::Kind::CppSource
				? AnyBody { copy_string(ctx.arena, ast.body.cpp_source()) }
				: AnyBody { ctx.arena.put(check_function_body(ast.body.expression(), ctx, funs_table, structs_table, fun, builtin_types)) };
		});
	}

	const StringSlice BOOL = StringSlice { "Bool" };
	const StringSlice STRING = StringSlice { "String" };
	const StringSlice VOID = StringSlice { "Void" };

	Option<Type> get_builtin_type(const StructsTable& structs_table, CheckCtx& al, StringSlice type_name) {
		return map_option<Type>()(structs_table.get(type_name), [&](Ref<const StructDeclaration> strukt) -> Option<Type> {
			if (strukt->arity()) {
				al.diag(strukt->range, Diag::Kind::SpecialTypeShouldNotHaveTypeParameters);
				return {};
			} else if (!strukt->copy) {
				al.diag(strukt->range, Diag::Kind::SpecialTypeShouldNotHaveTypeParameters); //TODO: SpecialTypeShouldBeCopy
				return {};
			} else {
				if (type_name == VOID) {
					assert(strukt->body.is_fields() && strukt->body.fields().is_empty()); //TODO: diagnostic for this
				}
				return Option { Type::noborrow(StoredType { InstStruct { strukt, {} } }) };
			}
		});
	}
}

void check(Ref<Module> m, Option<BuiltinTypes>& builtin_types, const FileAst& ast, Arena& arena, ListBuilder<Diagnostic>& diagnostics) {
	CheckCtx ctx { arena, ast.source, m->path, m->imports, diagnostics };

	if (!ast.imports.is_empty()) todo();

	check_type_headers(ast, ctx, m);

	check_fun_headers_and_type_bodies(ast, ctx,
		m->structs_declaration_order, m->specs_declaration_order, m->funs_declaration_order,
		m->structs_table, m->specs_table, m->funs_table);

	if (!builtin_types.has())
		builtin_types = BuiltinTypes { get_builtin_type(m->structs_table, ctx, BOOL), get_builtin_type(m->structs_table, ctx, STRING), get_builtin_type(m->structs_table, ctx, VOID) };

	check_fun_bodies(ast, ctx, builtin_types.get(), m->funs_declaration_order, m->structs_table, m->funs_table);
}
