#include "check.h"

#include "./check_expr.h"
#include "./convert_type.h"

namespace {
	//TODO
	Identifier id(Arena& arena, StringSlice s) {
		return Identifier { arena.str(s) };
	}

	DynArray<TypeParameter> check_type_parameters(const DynArray<TypeParameterAst> asts, Arena& arena) {
		return arena.map_array<TypeParameter>()(asts, [&](const TypeParameterAst& ast) {
			//TODO: check for duplicate names
			return TypeParameter { id(arena, ast.name), ast.index };
		});
	}

	DynArray<Parameter> check_parameters(const DynArray<ParameterAst>& asts, Arena& arena, const StructsTable& structs_table, const TypeParametersScope& type_parameters_scope) {
		return arena.map_array<Parameter>()(asts, [&](const ParameterAst& ast) {
			//TODO: check for duplicate names
			return Parameter { type_from_ast(ast.type, arena, structs_table, type_parameters_scope), id(arena, ast.name) };
		});
	}

	DynArray<SpecUse> check_spec_uses(const DynArray<SpecUseAst>& asts, Arena& arena, const StructsTable& structs_table, const SpecsTable& specs_table, const TypeParametersScope& type_parameters_scope) {
		return arena.map_array<SpecUse>()(asts, [&](const SpecUseAst& ast) {
			Option<const ref<const SpecDeclaration>&> spec_op = specs_table.get(ast.spec);
			if (!spec_op) throw "todo";
			ref<const SpecDeclaration> spec = spec_op.get();
			DynArray<Type> args = type_arguments_from_asts(ast.type_arguments, arena, structs_table, type_parameters_scope);
			if (args.size() != spec->type_parameters.size()) throw "todo";
			return SpecUse { spec, args };
		});
	}

	FunSignature check_signature(const FunSignatureAst& ast, Arena& arena, const StructsTable& structs_table, const SpecsTable& specs_table, const DynArray<TypeParameter>& spec_type_parameters) {
		DynArray<TypeParameter> type_parameters = check_type_parameters(ast.type_parameters, arena);
		TypeParametersScope type_parameters_scope { spec_type_parameters, type_parameters };
		Type return_type = type_from_ast(ast.return_type, arena, structs_table, type_parameters_scope);
		Identifier name = id(arena, ast.name);
		DynArray<Parameter> parameters = check_parameters(ast.parameters, arena, structs_table, type_parameters_scope);
		DynArray<SpecUse> specs = check_spec_uses(ast.specs, arena, structs_table, specs_table, type_parameters_scope);
		return { type_parameters, return_type, name, parameters, specs };
	}


	DynArray<StructField> check_struct_fields(const DynArray<StructFieldAst>& asts, Arena& arena, const StructsTable& structs_table, const DynArray<TypeParameter>& struct_type_parameters) {
		TypeParametersScope type_parameters_scope { {}, struct_type_parameters };
		return arena.map_array<StructField>()(asts, [&](const StructFieldAst& field) {
			return StructField { type_from_ast(field.type, arena, structs_table, type_parameters_scope), id(arena, field.name) };
		});
	}

	void add_overload(FunsTable& funs_table, ref<FunDeclaration> f) {
		funs_table.add(f->name().str, f);
	}

	// Parses the *existence* of each type and function -- does not parse the contents.
	// Meaning, for a struct we just *count* fields, and for a sig we just *count* signatures.
	// When this function is finished, we should have allocated all structs and sigs.
	void check_type_headers(
		const std::vector<DeclarationAst>& declarations, Arena& arena, ref<const Module> containing_module,
		StructsDeclarationOrder& structs, SpecsDeclarationOrder& specs,
		StructsTable& structs_table, SpecsTable& specs_table) {

		for (const DeclarationAst& decl : declarations) {
			switch (decl.kind()) {
				case DeclarationAst::Kind::CppInclude:
					throw "todo";

				case DeclarationAst::Kind::Struct: {
					const StructDeclarationAst& ast = decl.strukt();
					ref<StructDeclaration> s = arena.emplace<StructDeclaration>()(containing_module, check_type_parameters(ast.type_parameters, arena), id(arena, ast.name));
					structs.push_back(s);
					bool inserted = structs_table.try_insert(s->name.str, s);
					if (!inserted) throw "todo";
					break;
				}

				case DeclarationAst::Kind::Spec: {
					const SpecDeclarationAst& ast = decl.spec();
					ref<SpecDeclaration> s = arena.emplace<SpecDeclaration>()(containing_module, check_type_parameters(ast.type_parameters, arena), id(arena, ast.name));
					specs.push_back(s);
					bool inserted = specs_table.try_insert(s->name.str, s);
					if (!inserted) throw "todo";
					break;
				}

				case DeclarationAst::Kind::Fun:
					// Ignore for now
					//funs.push_back(arena.emplace<FunDeclaration>()(containing_module, type_parameters););
					break;
			}
		}
	}

	template <typename T>
	T& get_and_increment(typename std::vector<T>::iterator& it, const typename std::vector<T>::iterator& end) {
		assert(it != end);
		T& res = *it;
		++it;
		return res;
	}

	// Now that we've allocated every struct and spec, fill in every struct and spec, and fill in the header of every function.
	// Can't check function bodies at this point as it may call a future function -- we need to get the headers of every function first.
	void check_fun_headers_and_type_bodies(
		const std::vector<DeclarationAst>& declarations, Arena& arena, ref<const Module> containing_module,
		StructsDeclarationOrder& structs, SpecsDeclarationOrder& specs, FunsDeclarationOrder& funs,
		const StructsTable& structs_table, const SpecsTable& specs_table, FunsTable& funs_table
	) {
		StructsDeclarationOrder::iterator struct_iter = structs.begin();
		StructsDeclarationOrder::iterator struct_end = structs.end();
		SpecsDeclarationOrder::iterator specs_iter = specs.begin();
		SpecsDeclarationOrder::iterator specs_end = specs.end();

		for (const DeclarationAst& decl : declarations) {
			switch (decl.kind()) {
				case DeclarationAst::Kind::CppInclude:
					throw "todo";

				case DeclarationAst::Kind::Struct: {
					const StructBodyAst& body_ast = decl.strukt().body;
					ref<StructDeclaration> strukt = get_and_increment<ref<StructDeclaration>>(struct_iter, struct_end);
					strukt->body = body_ast.kind() == StructBodyAst::Kind::CppName
					   ? StructBody{arena.str(body_ast.cpp_name())}
					   : StructBody{check_struct_fields(body_ast.fields(), arena, structs_table, strukt->type_parameters)};
					break;
				}

				case DeclarationAst::Kind::Spec: {
					ref<SpecDeclaration> spec = get_and_increment<ref<SpecDeclaration>>(specs_iter, specs_end);
					spec->signatures = arena.map_array<FunSignature>()(decl.spec().signatures, [&](const FunSignatureAst& ast) {
						return check_signature(ast, arena, structs_table, specs_table, spec->type_parameters);
					});
					break;
				}

				case DeclarationAst::Kind::Fun: {
					const FunDeclarationAst& fun_ast = decl.fun();
					ref<FunDeclaration> fun = arena.emplace_copy(FunDeclaration { containing_module, check_signature(fun_ast.signature, arena, structs_table, specs_table, {}), {} });
					funs.push_back(fun);
					add_overload(funs_table, fun);
					break;
				}
			}
		}

		assert(struct_iter == struct_end && specs_iter == specs_end);
	}

	const StringSlice BOOL = StringSlice { "Bool" };
	const StringSlice STRING = StringSlice { "String" };
	const StringSlice VOID = StringSlice { "Void" };

	Option<Type> get_special_named_type(const StructsTable& structs_table, StringSlice type_name) {
		return map<const ref<const StructDeclaration>&, Type>(structs_table.get(type_name))([](ref<const StructDeclaration> rf) -> Type {
			if (rf->arity()) throw "todo: Bool/String shouldn't have type parameters";
			return Type { PlainType { Effect::Pure, { rf, {} } } };
		});
	}

	Expression check_function_body(const ExprAst& ast, Arena& arena, const FunsTable& funs_table, const StructsTable& structs_table, const FunDeclaration& fun, const BuiltinTypes& builtin_types) {
		Arena scratch_arena;
		ExprContext ctx { arena, scratch_arena, funs_table, structs_table, &fun, {}, builtin_types };
		return check_and_expect(ast, ctx, fun.signature.return_type);
	}

	// Now that we have the bodies of every type and the headers of every function, we can fill in every function.
	void check_fun_bodies(const std::vector<DeclarationAst>& declarations, Arena& arena, FunsDeclarationOrder& funs, const StructsTable& structs_table, const FunsTable& funs_table) {
		FunsDeclarationOrder::iterator fun_iter = funs.begin();
		FunsDeclarationOrder::iterator fun_end = funs.end();
		BuiltinTypes builtin_types { get_special_named_type(structs_table, BOOL), get_special_named_type(structs_table, STRING), get_special_named_type(structs_table, VOID) };

		for (const DeclarationAst& decl : declarations) {
			switch (decl.kind()) {
				case DeclarationAst::Kind::CppInclude:
				case DeclarationAst::Kind::Struct:
				case DeclarationAst::Kind::Spec:
					break;
				case DeclarationAst::Kind::Fun: {
					const FunDeclarationAst& fun_ast = decl.fun();
					ref<FunDeclaration> fun = get_and_increment<ref<FunDeclaration>>(fun_iter, fun_end);
					fun->body = fun_ast.body.kind() == FunBodyAst::Kind::CppSource
						? AnyBody{arena.str(fun_ast.body.cpp_source())}
						: AnyBody{arena.emplace_copy(check_function_body(fun_ast.body.expression(), arena, funs_table, structs_table, fun, builtin_types))};
				}

			}
		}
	}
}

ref<Module> check(const StringSlice& file_path, const Identifier& module_name, const std::vector<DeclarationAst>& declarations, Arena& arena) {
	ref<Module> m = arena.emplace<Module>()(arena.str(file_path), module_name);
	check_type_headers(declarations, arena, m, m->structs_declaration_order, m->specs_declaration_order, m->structs_table, m->specs_table);
	check_fun_headers_and_type_bodies(declarations, arena, m,
		m->structs_declaration_order, m->specs_declaration_order, m->funs_declaration_order,
		m->structs_table, m->specs_table, m->funs_table);
	check_fun_bodies(declarations, arena, m->funs_declaration_order, m->structs_table, m->funs_table);
	return m;
}
