#include "check.h"

#include "./check_effects.h"
#include "./check_expr.h"
#include "./convert_type.h"
#include "./scope.h"

namespace {
	Arr<TypeParameter> check_type_parameters(const Arr<TypeParameterAst> asts, CheckCtx& al, const Arr<TypeParameter>& spec_type_parameters) {
		return al.arena.map_with_prevs<TypeParameter>()(asts, [&](const TypeParameterAst& ast, const Arr<TypeParameter>& prevs, uint index) {
			if (some(spec_type_parameters, [&](const TypeParameter& s) { return s.name == ast.name; }))
				al.diag(ast.name, Diag::Kind::TypeParameterShadowsSpecTypeParameter);
			for (const TypeParameter& prev : prevs)
				if (prev.name == ast.name)
					al.diag(ast.name, Diag::Kind::TypeParameterShadowsPrevious);
			assert(index == ast.index);
			return TypeParameter { al.range(ast.name), id(al, ast.name), index };
		});
	}

	bool type_is_copy(const Type& type) {
		return type.is_inst_struct() && type.inst_struct().strukt->copy;
	}

	Effect check_parameter_effect(const Option<Effect>& declared, const Type& type) {
		if (!declared.has()) return Effect::EGet; // Yes, even for 'copy' types.

		Effect e = declared.get();
		switch (e) {
			case Effect::EGet:
				throw "todo";
			case Effect::ESet:
			case Effect::EIo:
				break;
			case Effect::EOwn:
				if (type_is_copy(type)) throw "todo"; //diagnostic: This isn't really necessary. We'll copy when we need it.
				break;
		}
		return e;
	}
	Effect check_return_effect(const Option<Effect>& declared, const Arr<Parameter>& parameters) {
		bool some_from = some(parameters, [](const Parameter& p) { return p.from; });
		if (some_from) {
			if (declared.has()) {
				assert(declared.get() != Effect::EOwn);
				if (declared.get() == Effect::EIo)
					throw "todo"; // This is pointless since EIo is the maximum by-reference effect anyway.
				return declared.get();
			} else {
				return Effect::EIo;
			}
		} else {
			if (declared.has())
				throw "todo"; // Can't declare an effect, since returning by value
			return Effect::EOwn;
		}
	}

	Arr<Parameter> check_parameters(
		const Arr<ParameterAst>& asts, CheckCtx& al,
		const StructsTable& structs_table, const TypeParametersScope& type_parameters_scope, const FunsTable& funs_table, const Arr<SpecUse>& current_specs) {
		return al.arena.map_with_prevs<Parameter>()(asts, [&](const ParameterAst& ast, const Arr<Parameter>& prevs, uint index) -> Parameter {
			check_param_or_local_shadows_fun(al, ast.name, funs_table, current_specs);
			if (some(prevs, [&](const Parameter& prev) { return prev.name == ast.name; }))
				throw "todo";
			Type type = type_from_ast(ast.type, al, structs_table, type_parameters_scope);
			return Parameter { ast.from, check_parameter_effect(ast.effect, type), type, id(al, ast.name), index };
		});
	}

	Arr<SpecUse> check_spec_uses(const Arr<SpecUseAst>& asts, CheckCtx& ctx, const StructsTable& structs_table, const SpecsTable& specs_table, const TypeParametersScope& type_parameters_scope) {
		return ctx.arena.map_op<SpecUse>()(asts, [&](const SpecUseAst& ast) -> Option<SpecUse> {
			Option<ref<const SpecDeclaration>> spec_op = find_spec(ast.spec, ctx, specs_table);
			if (!spec_op.has()) {
				ctx.diag(ast.spec, Diag::Kind::SpecNameNotFound);
				return {};
			};

			ref<const SpecDeclaration> spec = spec_op.get();
			Arr<Type> args = type_arguments_from_asts(ast.type_arguments, ctx, structs_table, type_parameters_scope);
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
		const Arr<TypeParameter>& spec_type_parameters, Identifier name
	) {
		Arr<TypeParameter> type_parameters = check_type_parameters(ast.type_parameters, ctx, spec_type_parameters);
		TypeParametersScope type_parameters_scope { spec_type_parameters, type_parameters };
		Type return_type = type_from_ast(ast.return_type, ctx, structs_table, type_parameters_scope);
		Arr<SpecUse> specs = check_spec_uses(ast.spec_uses, ctx, structs_table, specs_table, type_parameters_scope);
		Arr<Parameter> parameters = check_parameters(ast.parameters, ctx, structs_table, type_parameters_scope, funs_table, specs);
		return { ctx.copy_str(ast.comment), type_parameters, check_return_effect(ast.effect, parameters), return_type, name, parameters, specs };
	}

	Arr<StructField> check_struct_fields(const Arr<StructFieldAst>& asts, CheckCtx& ctx, const StructsTable& structs_table, const Arr<TypeParameter>& struct_type_parameters) {
		TypeParametersScope type_parameters_scope { {}, struct_type_parameters };
		return ctx.arena.map<StructField>()(asts, [&](const StructFieldAst& field) {
			return StructField { ctx.copy_str(field.comment), type_from_ast(field.type, ctx, structs_table, type_parameters_scope), id(ctx, field.name) };
		});
	}

	void check_type_headers(const FileAst& file_ast, CheckCtx& ctx, ref<Module> module) {

		if (!file_ast.includes.empty()) throw "todo";
		//for (const StringSlice& include __attribute__((unused)) : file_ast.includes) {
		//	throw "todo";
		//}

		module->specs_declaration_order = ctx.arena.map<SpecDeclaration>()(file_ast.specs, [&](const SpecDeclarationAst& ast) {
			return SpecDeclaration { module, ast.range, ctx.copy_str(ast.comment), ast.is_public, check_type_parameters(ast.type_parameters, ctx, {}), id(ctx, ast.name) };
		});
		//TODO: do this in the loop somehow -- but make sure you have the address to the spec *in the array*
		for (const SpecDeclaration& spec : module->specs_declaration_order)
			if (!module->specs_table.try_insert(spec.name, &spec))
				ctx.diag(spec.name, Diag::Kind::DuplicateDeclaration);

		module->structs_declaration_order = ctx.arena.map<StructDeclaration>()(file_ast.structs, [&](const StructDeclarationAst& ast) {
			return StructDeclaration { module, ast.range, ast.is_public, check_type_parameters(ast.type_parameters, ctx, {}), id(ctx, ast.name), ast.copy };
		});
		for (const StructDeclaration& strukt : module->structs_declaration_order)
			if (!module->structs_table.try_insert(strukt.name, &strukt))
				ctx.diag(strukt.range, Diag::Kind::DuplicateDeclaration);

		module->funs_declaration_order = ctx.arena.map<FunDeclaration>()(file_ast.funs, [&](const FunDeclarationAst& ast) {
			return FunDeclaration { module, ast.is_public, { id(ctx, ast.signature.name) }, {} };
		});
		for (const FunDeclaration& fun : module->funs_declaration_order)
			// Unlike structs and specs, functions can overload.
			module->funs_table.add(fun.name(), &fun);
	}

	template <typename T, typename U, typename /*const T&, U& => void*/ Cb>
	void zipp(const Grow<T>& grow, Arr<U>& v, Cb cb) {
		assert(grow.size() == v.size());
		uint i = 0;
		for (const T& t : grow) {
			cb(t, v[i]);
			++i;
		}
	}

	// Now that we've allocated every struct and spec, fill in every struct and spec, and fill in the header of every function.
	// Can't check function bodies at this point as it may call a future function -- we need to get the headers of every function first.
	void check_fun_headers_and_type_bodies(
		const FileAst& file_ast, CheckCtx& al,
		StructsDeclarationOrder& structs, SpecsDeclarationOrder& specs, FunsDeclarationOrder& funs,
		const StructsTable& structs_table, const SpecsTable& specs_table, FunsTable& funs_table
	) {
		zipp(file_ast.specs, specs, [&](const SpecDeclarationAst& spec_ast, SpecDeclaration& spec) {
			spec.signatures = al.arena.map<FunSignature>()(spec_ast.signatures, [&](const FunSignatureAst& ast) {
				return check_signature(ast, al, structs_table, specs_table, funs_table, spec.type_parameters, id(al, ast.name));
			});
		});

		zipp(file_ast.structs, structs, [&](const StructDeclarationAst& struct_ast, StructDeclaration& strukt) {
			const StructBodyAst& body_ast = struct_ast.body;
			strukt.body = body_ast.kind() == StructBodyAst::Kind::CppName
						   ? StructBody{al.arena.str(body_ast.cpp_name())}
						   : StructBody{check_struct_fields(body_ast.fields(), al, structs_table, strukt.type_parameters)};
		});

		zipp(file_ast.funs, funs, [&](const FunDeclarationAst& fun_ast, FunDeclaration& fun) {
			// Name was allocated in the previous step.
			fun.signature = check_signature(fun_ast.signature, al, structs_table, specs_table, funs_table, {}, fun.signature.name);
		});
	}

	const StringSlice BOOL = StringSlice { "Bool" };
	const StringSlice STRING = StringSlice { "String" };
	const StringSlice VOID = StringSlice { "Void" };

	Option<Type> get_special_named_type(const StructsTable& structs_table, CheckCtx& al, StringSlice type_name) {
		return map_op<Type>()(structs_table.get(type_name), [&](ref<const StructDeclaration> strukt) -> Type {
			if (strukt->arity()) {
				al.diag(strukt->range, Diag::Kind::SpecialTypeShouldNotHaveTypeParameters);
				return {};
			} else
				return Type { InstStruct { strukt, {} } };
		});
	}

	Expression check_function_body(
		const ExprAst& ast, CheckCtx& al, const FunsTable& funs_table, const StructsTable& structs_table, const FunDeclaration& fun, const BuiltinTypes& builtin_types) {
		Arena scratch_arena;
		ExprContext ctx { al, scratch_arena, funs_table, structs_table, &fun, {}, builtin_types };
		return check_and_expect(ast, ctx, fun.signature.return_type);
	}

	// Now that we have the bodies of every type and the headers of every function, we can fill in every function.
	void check_fun_bodies(const FileAst& file_ast, CheckCtx& al, FunsDeclarationOrder& funs, const StructsTable& structs_table, const FunsTable& funs_table) {
		BuiltinTypes builtin_types { get_special_named_type(structs_table, al, BOOL), get_special_named_type(structs_table, al, STRING), get_special_named_type(structs_table, al, VOID) };
		zipp(file_ast.funs, funs, [&](const FunDeclarationAst& ast, FunDeclaration& fun) {
			if (ast.body.kind() == FunBodyAst::Kind::CppSource)
				fun.body = AnyBody{ al.arena.str(ast.body.cpp_source())};
			else {
				fun.body = AnyBody { al.arena.put(check_function_body(ast.body.expression(), al, funs_table, structs_table, fun, builtin_types)) };
				check_effects(fun);
			}
		});
	}
}

void check(ref<Module> m, const FileAst& ast, Arena& arena, Grow<Diagnostic>& diagnostics) {
	CheckCtx ctx { arena, ast.source, m->path, m->imports, diagnostics };

	if (!ast.imports.empty()) throw "todo";

	check_type_headers(ast, ctx, m);

	check_fun_headers_and_type_bodies(ast, ctx,
		m->structs_declaration_order, m->specs_declaration_order, m->funs_declaration_order,
		m->structs_table, m->specs_table, m->funs_table);

	check_fun_bodies(ast, ctx, m->funs_declaration_order, m->structs_table, m->funs_table);
}
