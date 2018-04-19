#include <unordered_map>

#include "parser.h"

#include "./Lexer.h"
#include "./check/convert_type.h"
#include "./check/check_function_body.h"

namespace {
	DynArray<TypeParameter> parse_type_parameters(Lexer& lexer, Arena& arena) {
		lexer.skip_blank_lines();
		if (!lexer.try_take('<')) return {};

		Arena::SmallArrayBuilder<TypeParameter> parameters = arena.small_array_builder<TypeParameter>();
		uint index = 0;
		do {
			parameters.add({ lexer.take_type_name(arena), index });
			++index;
		} while (lexer.try_take_comma_space());
		lexer.take('>');
		lexer.take(' ');
		return parameters.finish();
	}
	void skip_type_parameters(Lexer& lexer) {
		lexer.skip_blank_lines();
		if (!lexer.try_take('<')) return;
		do { lexer.skip_type_name(); } while (lexer.try_take_comma_space());
		lexer.take('>');
		lexer.take(' ');
	}


	DynArray<Parameter> parse_parameters(Lexer& lexer, Arena& arena, const StructsTable& structs_table, const TypeParametersScope& type_parameters_scope) {
		Arena::SmallArrayBuilder<Parameter> parameters = arena.small_array_builder<Parameter>();
		lexer.take('(');
		if (!lexer.try_take(')')) {
			while (true) {
				Type param_type = parse_type(lexer, arena, structs_table, type_parameters_scope);
				lexer.take(' ');
				parameters.add({ param_type, Identifier{arena.str(lexer.take_value_name())} });
				if (lexer.try_take(')')) break;
				lexer.take(',');
				lexer.take(' ');
			}
		}
		return parameters.finish();
	}

	DynArray<SpecUse> parse_spec_uses(Lexer& lexer, Arena& arena, const StructsTable& structs_table, const SpecsTable& specs_table, const TypeParametersScope& type_parameters_scope) {
		if (!lexer.try_take(' ')) return {};

		Arena::SmallArrayBuilder<SpecUse> spec_uses = arena.small_array_builder<SpecUse>();
		do {
			StringSlice name = lexer.take_spec_name();
			Option<const ref<const SpecDeclaration>&> spec_op = specs_table.get(name);
			if (!spec_op) throw "todo";
			ref<const SpecDeclaration> spec = spec_op.get();
			DynArray<Type> args = parse_type_arguments(lexer, arena, structs_table, type_parameters_scope);
			if (args.size() != spec->type_parameters.size()) throw "todo";
			spec_uses.add({ spec, args });
		} while (lexer.try_take(' '));
		return spec_uses.finish();
	}

	void parse_signature(Lexer& lexer, Arena& arena, FunSignature& sig, const StructsTable& structs_table, const SpecsTable& specs_table, const DynArray<TypeParameter>& spec_type_parameters) {
		TypeParametersScope type_parameters_scope { spec_type_parameters, sig.type_parameters };
		sig.return_type = parse_type(lexer, arena, structs_table, type_parameters_scope);
		lexer.take(' ');
		sig.name = Identifier{arena.str(lexer.take_value_name())};
		sig.parameters = parse_parameters(lexer, arena, structs_table, type_parameters_scope);
		sig.specs = parse_spec_uses(lexer, arena, structs_table, specs_table, type_parameters_scope);
	}

	DynArray<StructField> parse_struct_fields(Lexer& lexer, StructDeclaration& s, Arena& arena, const StructsTable& structs_table) {
		lexer.take_indent();
		Arena::SmallArrayBuilder<StructField> b = arena.small_array_builder<StructField>();
		TypeParametersScope type_parameters_scope { {}, s.type_parameters };
		do {
			Type type = parse_type(lexer, arena, structs_table, type_parameters_scope);
			lexer.take(' ');
			b.add({ type, Identifier{arena.str(lexer.take_value_name())} });
		} while (lexer.take_newline_or_dedent() == NewlineOrDedent::Newline);
		return b.finish();
	}

	DynArray<FunSignature> parse_spec(Lexer& lexer, Arena& arena, const StructsTable& structs_table, const SpecsTable& specs_table, const DynArray<TypeParameter>& spec_type_parameters) {
		lexer.take_indent();
		Arena::SmallArrayBuilder<FunSignature> sigs = arena.small_array_builder<FunSignature>();
		do {
			FunSignature sig(parse_type_parameters(lexer, arena));
			for (const TypeParameter& t : sig.type_parameters)
				if (some(spec_type_parameters, [&](const TypeParameter& spec_type_parameter) { return spec_type_parameter.name == t.name; }))
					throw "Todo";

			parse_signature(lexer, arena, sig, structs_table, specs_table, spec_type_parameters);
			sigs.add(sig);
		} while (lexer.take_newline_or_dedent() == NewlineOrDedent::Newline);
		return sigs.finish();
	}

	void add_overload(FunsTable& funs_table, ref<FunDeclaration> f) {
		funs_table.add(f->name().str, f);
	}

	StructBody parse_cpp_struct_body(Lexer& lexer, Arena& arena) {
		if (lexer.try_take(' ')) {
			lexer.take('=');
			lexer.take(' ');
			StringSlice cpp_name = lexer.take_cpp_type_name();
			return StructBody(StructBody::Kind::CppName, arena.str(cpp_name));
		} else {
			return StructBody(StructBody::Kind::CppBody, lexer.take_indented_string(arena));
		}
	}

	// Parses the *existence* of each type and function -- does not parse the contents.
	// Meaning, for a struct we just *count* fields, and for a sig we just *count* signatures.
	// When this function is finished, we should have allocated all structs and sigs.
	void parse_type_headers(
		Lexer& lexer, Arena& arena, ref<const Module> containing_module,
		StructsDeclarationOrder& structs, SpecsDeclarationOrder& sigs,
		StructsTable& structs_table, SpecsTable& sigs_table, FunsDeclarationOrder& funs) {
		while (true) {
			DynArray<TypeParameter> type_parameters = parse_type_parameters(lexer, arena);
			switch (lexer.try_take_top_level_keyword()) {
				case TopLevelKeyword::KwCppInclude:
					throw "todo";

				case TopLevelKeyword::KwStruct:
				case TopLevelKeyword::KwCppStruct: {
					lexer.take(' ');
					Identifier name = lexer.take_type_name(arena);
					ref<StructDeclaration> s = arena.emplace<StructDeclaration>()(containing_module, name, type_parameters);
					structs.push_back(s);
					bool inserted = structs_table.try_insert(s->name.str, s);
					if (!inserted) throw "todo";
					break;
				}

				case TopLevelKeyword::KwSpec: {
					lexer.take(' ');
					Identifier name = lexer.take_spec_name(arena);
					ref<SpecDeclaration> s = arena.emplace<SpecDeclaration>()(containing_module, type_parameters, name);
					sigs.push_back(s);
					bool inserted = sigs_table.try_insert(s->name.str, s);
					if (!inserted) throw "todo";
					break;
				}

				case TopLevelKeyword::KwCpp: // cpp function
				case TopLevelKeyword::None: { // function
					ref<FunDeclaration> f = arena.emplace<FunDeclaration>()(containing_module, type_parameters);
					funs.push_back(f);
					break;
				}

				case TopLevelKeyword::KwEof:
					if (type_parameters.size()) throw "todo";
					return;
			}

			lexer.skip_to_end_of_line_and_optional_indented_lines();
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
	void parse_fun_headers_and_type_bodies(
		Lexer& lexer, Arena& arena,
		StructsDeclarationOrder& structs, SpecsDeclarationOrder& specs, FunsDeclarationOrder& funs,
		const StructsTable& structs_table, const SpecsTable& specs_table, FunsTable& funs_table
	) {
		StructsDeclarationOrder::iterator struct_iter = structs.begin();
		StructsDeclarationOrder::iterator struct_end = structs.end();
		SpecsDeclarationOrder::iterator specs_iter = specs.begin();
		SpecsDeclarationOrder::iterator specs_end = specs.end();
		FunsDeclarationOrder::iterator fun_iter = funs.begin();
		FunsDeclarationOrder::iterator fun_end = funs.end();

		while (true) {
			skip_type_parameters(lexer);
			TopLevelKeyword kw = lexer.try_take_top_level_keyword();
			switch (kw) {
				case TopLevelKeyword::KwCppInclude:
					// Already handled
					lexer.skip_to_end_of_line_and_newline();
					break;

				case TopLevelKeyword::KwCppStruct:
				case TopLevelKeyword::KwStruct: {
					lexer.take(' ');
					lexer.skip_type_name();
					ref<StructDeclaration> strukt = get_and_increment<ref<StructDeclaration>>(struct_iter, struct_end);
					strukt->body = kw == TopLevelKeyword::KwStruct ? parse_struct_fields(lexer, *strukt, arena, structs_table) : parse_cpp_struct_body(lexer, arena);
					break;
				}

				case TopLevelKeyword::KwSpec: {
					lexer.take(' ');
					lexer.skip_spec_name();
					ref<SpecDeclaration> spec = get_and_increment<ref<SpecDeclaration>>(specs_iter, specs_end);
					spec->signatures = parse_spec(lexer, arena, structs_table, specs_table, spec->type_parameters);
					break;
				}

				case TopLevelKeyword::KwCpp: // cpp function
				case TopLevelKeyword::None: { // function
					ref<FunDeclaration> fun = get_and_increment<ref<FunDeclaration>>(fun_iter, fun_end);
					parse_signature(lexer, arena, fun->signature, structs_table, specs_table, /*spec_type_parameters*/ {});
					add_overload(funs_table, fun);
					lexer.skip_indent_and_indented_lines();
					break;
				}

				case TopLevelKeyword::KwEof:
					assert(struct_iter == struct_end && specs_iter == specs_end && fun_iter == fun_end);
					return;
			}
		}
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

	// Now that we have the bodies of every type and the headers of every function, we can fill in every function.
	void parse_fun_bodies(Lexer& lexer, Arena& arena, FunsDeclarationOrder& funs, const StructsTable& structs_table, const FunsTable& funs_table) {
		FunsDeclarationOrder::iterator fun_iter = funs.begin();
		FunsDeclarationOrder::iterator fun_end = funs.end();
		BuiltinTypes builtin_types { get_special_named_type(structs_table, BOOL), get_special_named_type(structs_table, STRING), get_special_named_type(structs_table, VOID) };
		Arena scratch_arena;

		while (true) {
			skip_type_parameters(lexer);
			TopLevelKeyword kw = lexer.try_take_top_level_keyword();
			switch (kw) {
				case TopLevelKeyword::KwCppInclude:
					// Already handled
					lexer.skip_to_end_of_line_and_newline();
					break;

				case TopLevelKeyword::KwCppStruct:
				case TopLevelKeyword::KwStruct:
				case TopLevelKeyword::KwSpec:
					// Already finished these in the last step.
					lexer.skip_to_end_of_line_and_optional_indented_lines();
					break;

				case TopLevelKeyword::KwCpp: // cpp function
				case TopLevelKeyword::None: { // function
					lexer.skip_to_end_of_line();
					ref<FunDeclaration> fun = get_and_increment<ref<FunDeclaration>>(fun_iter, fun_end);
					if (kw == TopLevelKeyword::KwCpp) {
						fun->body = lexer.take_indented_string(arena);
					} else {
						lexer.skip_to_end_of_line();
						lexer.take_indent();
						fun->body = arena.emplace_copy(check_function_body(lexer, arena, scratch_arena, funs_table, structs_table, fun, builtin_types));
						scratch_arena.clear();
					}
					break;
				}

				case TopLevelKeyword::KwEof:
					assert(fun_iter == fun_end);
					return;
			}
		}
	}

	// May throw a ParseDiagnostic. We should only do this for top-level parsing failures, and within a function just add diagnostics.
	void parse_file_worker(const StringSlice& module_source, ref<Module> m, Arena& arena) {
		Lexer::validate_file(module_source);

		Lexer lexer { module_source };
		parse_type_headers(lexer, arena, m, m->structs_declaration_order, m->specs_declaration_order, m->structs_table, m->specs_table, m->funs_declaration_order);
		lexer.reset(module_source);
		parse_fun_headers_and_type_bodies(
			lexer, arena, m->structs_declaration_order, m->specs_declaration_order, m->funs_declaration_order, m->structs_table, m->specs_table, m->funs_table);
		lexer.reset(module_source);
		parse_fun_bodies(lexer, arena, m->funs_declaration_order, m->structs_table, m->funs_table);
	}
}


ref<Module> parse_file(const StringSlice& file_path, const Identifier& module_name, const StringSlice& file_content, Arena& arena) {
	ref<Module> module = arena.emplace<Module>()(arena.str(file_path), module_name);

	try {
		parse_file_worker(file_content, module, arena);
	} catch (ParseDiagnostic p) {
		// This should only happen for a truly fatal error, so all structs as they may not be fully initialized yet.
		module->diagnostics = arena.make_list<Diagnostic>(p);
		// If we did allocate any structs, remove them.
		module->structs_declaration_order.clear();
		module->funs_declaration_order.clear();
		module->structs_table.clear();
		module->funs_table.clear();
		return module;
	}

	return module;
}
