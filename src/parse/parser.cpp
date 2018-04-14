#include <unordered_map>

#include "parser.h"

#include "Lexer.h"
#include "../util/collection_util.h"

#include "check/check_function_body.h"

namespace {
	Type parse_type(Lexer& lexer, Arena& arena, const StructsTable& structs_table, const DynArray<TypeParameter>& type_parameters_in_scope);

	TypeParameter parse_type_parameter(Lexer& lexer, Arena& arena, uint index) {
		StringSlice name = lexer.take_type_name();
		return { Identifier{arena.str(name)}, index };
	}

	DynArray<TypeParameter> parse_type_parameters(Lexer& lexer, Arena& arena) {
		if (!lexer.try_take('<')) return {};

		auto parameters = arena.small_array_builder<TypeParameter>();
		uint index = 0;
		do {
			parameters.add(parse_type_parameter(lexer, arena, index));
			++index;
		} while (lexer.try_take_comma_space());
		lexer.take('>');
		lexer.take(' ');
		return parameters.finish();
	}

	DynArray<Type> parse_type_arguments(size_t arity, Lexer& lexer, Arena& arena, const StructsTable& structs_table, const DynArray<TypeParameter>& type_parameters_in_scope) {
		if (arity == 0) return {};
		lexer.take('<');
		return arena.fill_array<Type>(arity)([&](uint i) {
			Type t = parse_type(lexer, arena, structs_table, type_parameters_in_scope);
			if (i == arity - 1) lexer.take('>'); else lexer.try_take_comma_space();
			return t;
		});
	}

	Type parse_type(Lexer& lexer, Arena& arena, const StructsTable& structs_table, const DynArray<TypeParameter>& type_parameters_in_scope) {
		Effect effect = lexer.try_take_effect();
		StringSlice name = lexer.take_type_name();
		Option<const TypeParameter&> tp = find(type_parameters_in_scope, [name](const TypeParameter& t) { return t.name == name; });
		if (tp)
			return ref<const TypeParameter>(&tp.get());
		Option<const ref<const StructDeclaration>&> op_type = structs_table.get(name);
		if (!op_type)
			throw "todo";
		ref<const StructDeclaration> strukt = op_type.get();
		return { effect, { strukt, parse_type_arguments(strukt->arity(), lexer, arena, structs_table, type_parameters_in_scope) } };
	}

	DynArray<StructField> parse_struct_fields(Lexer& lexer, StructDeclaration& s, Arena& arena, const StructsTable& structs_table) {
		lexer.take_indent();
		Arena::SmallArrayBuilder<StructField> b = arena.small_array_builder<StructField>();
		do {
			Type type = parse_type(lexer, arena, structs_table, s.type_parameters);
			lexer.take(' ');
			b.add({ type, Identifier{arena.str(lexer.take_value_name())} });
		} while (lexer.take_newline_or_dedent() == NewlineOrDedent::Newline);
		return b.finish();
	}

	void add_overload(FunsTable& funs_table, ref<Fun> f) {
		OverloadGroup& group = funs_table.get_or_create(f->name.str); // Creates a new empty group if necessary
		group.funs.push_back(f); // TODO: warn if adding same signature twice
	}

	void parse_fun_header_skip_body(Lexer& lexer, Arena& arena, Fun& fun, const StructsTable& structs_table, FunsTable& funs_table) {
		fun.return_type = parse_type(lexer, arena, structs_table, fun.type_parameters);
		lexer.take(' ');
		fun.name = Identifier{arena.str(lexer.take_value_name())};
		add_overload(funs_table, &fun);

		Arena::SmallArrayBuilder<Parameter> parameters = arena.small_array_builder<Parameter>();

		lexer.take('(');
		if (!lexer.try_take(')')) {
			while (true) {
				Type param_type = parse_type(lexer, arena, structs_table, fun.type_parameters);
				lexer.take(' ');
				parameters.add({ param_type, Identifier{arena.str(lexer.take_value_name())} });
				if (lexer.try_take(')')) break;
				lexer.take(',');
				lexer.take(' ');
			}
		}

		fun.parameters = parameters.finish();

		lexer.skip_indent_and_indented_lines();
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
	void parse_type_headers(Lexer& lexer, Arena& arena, ref<const Module> containing_module, StructsDeclarationOrder& structs, StructsTable& structs_table, FunsDeclarationOrder& funs) {
		DynArray<TypeParameter> type_parameters = parse_type_parameters(lexer, arena);
		while (true) {
			switch (lexer.try_take_top_level_keyword()) {
				case TopLevelKeyword::KwCppInclude:
					throw "todo";

				case TopLevelKeyword::KwStruct:
				case TopLevelKeyword::KwCppStruct: {
					lexer.take(' ');
					StringSlice name = lexer.take_type_name();
					ref<StructDeclaration> s = arena.emplace<StructDeclaration>()(containing_module, Identifier { arena.str(name) }, type_parameters);
					structs.push_back(s);
					bool inserted = structs_table.try_insert(s->name.str, s);
					if (!inserted) throw "todo";
					break;
				}

				case TopLevelKeyword::KwSig:
					throw "todo";

				case TopLevelKeyword::KwCpp: // cpp function
				case TopLevelKeyword::None: { // function
					ref<Fun> f = arena.emplace<Fun>()(containing_module, type_parameters);
					funs.push_back(f);
					break;
				}

				case TopLevelKeyword::KwEof:
					return;
			}

			lexer.skip_to_end_of_line_and_optional_indented_lines();
		}
	}

	// Now that we've allocated every struct and sig, fill in every struct and sig, and fill in the header of every function.
	// Can't check function bodies at this point as it may call a future function -- we need to get the headers of every function first.
	void parse_fun_headers_and_type_bodies(Lexer& lexer, Arena& arena, StructsDeclarationOrder& structs, FunsDeclarationOrder& funs, StructsTable& structs_table, FunsTable& funs_table) {
		auto struct_iter = structs.begin();
		auto struct_end = structs.end();
		auto fun_iter = funs.begin();
		auto fun_end = funs.end();

		while (true) {
			TopLevelKeyword kw = lexer.try_take_top_level_keyword();
			switch (kw) {
				case TopLevelKeyword::KwCppInclude:
					// Already handled
					lexer.skip_to_end_of_line_and_newline();
					break;

				case TopLevelKeyword::KwCppStruct:
				case TopLevelKeyword::KwStruct: {
					assert(struct_iter != struct_end);
					ref<StructDeclaration> s = *struct_iter;
					lexer.take(' ');
					lexer.skip_type_name();
					s->body = kw == TopLevelKeyword::KwStruct ? parse_struct_fields(lexer, *s, arena, structs_table) : parse_cpp_struct_body(lexer, arena);
					++struct_iter;
					break;
				}

				case TopLevelKeyword::KwSig:
					throw "todo";

				case TopLevelKeyword::KwCpp: // cpp function
				case TopLevelKeyword::None: // function
					assert(fun_iter != fun_end);
					parse_fun_header_skip_body(lexer, arena, *fun_iter, structs_table, funs_table);
					++fun_iter;
					break;

				case TopLevelKeyword::KwEof:
					return;
			}
		}

		assert(struct_iter == struct_end && fun_iter == fun_end);
	}

	const StringSlice BOOL = StringSlice { "Bool" };
	const StringSlice STRING = StringSlice { "String" };
	const StringSlice VOID = StringSlice { "Void" };

	Option<Type> get_special_named_type(const StructsTable& structs_table, StringSlice type_name) {
		return map<const ref<const StructDeclaration>&, Type>(structs_table.get(type_name))([](ref<const StructDeclaration> rf) -> Type {
			if (rf->arity()) throw "todo: Bool/String shouldn't have type parameters";
			return { Effect::Pure, { rf, {} } };
		});
	}

	// Now that we have the bodies of every type and the headers of every function, we can fill in every function.
	void parse_fun_bodies(Lexer& lexer, Arena& arena, FunsDeclarationOrder& funs, const StructsTable& structs_table, const FunsTable& funs_table) {
		Option<Type> bool_type = get_special_named_type(structs_table, BOOL);
		Option<Type> string_type = get_special_named_type(structs_table, STRING);
		Option<Type> void_type = get_special_named_type(structs_table, VOID);

		auto fun_iter = funs.begin();
		auto fun_end = funs.end();
		auto eat_fun = [&fun_iter, fun_end]() -> Fun& {
			assert(fun_iter != fun_end);
			ref<Fun> res = *fun_iter;
			++fun_iter;
			return *res;
		};

		Arena scratch_arena;

		while (true) {
			switch (lexer.try_take_top_level_keyword()) {
				case TopLevelKeyword::KwCppInclude:
					// Already handled
					lexer.skip_to_end_of_line_and_newline();
					break;

				case TopLevelKeyword::KwCppStruct:
				case TopLevelKeyword::KwStruct:
				case TopLevelKeyword::KwSig:
					// Already finished these in the last step.
					lexer.skip_to_end_of_line_and_optional_indented_lines();
					break;

				case TopLevelKeyword::KwCpp: // cpp function
					lexer.skip_to_end_of_line();
					eat_fun().body = lexer.take_indented_string(arena);
					break;

				case TopLevelKeyword::KwEof:
					assert(fun_iter == fun_end);
					return;

				case TopLevelKeyword::None: { // function
					lexer.skip_to_end_of_line();
					lexer.take_indent();
					Fun& fun = eat_fun();
					fun.body = check_function_body(lexer, arena, scratch_arena, funs_table, structs_table, fun.parameters, bool_type,
												   string_type, void_type, fun.return_type);
					scratch_arena.clear();
					break;
				}
			}
		}
	}

	// May throw a ParseDiagnostic. We should only do this for top-level parsing failures, and within a function just add diagnostics.
	void parse_file_worker(const StringSlice& module_source, ref<Module> module, Arena& arena) {
		Lexer::validate_file(module_source);

		Lexer lexer { module_source };
		parse_type_headers(lexer, arena, module, module->structs_declaration_order, module->structs_table, module->funs_declaration_order);
		lexer.reset(module_source);
		parse_fun_headers_and_type_bodies(lexer, arena, module->structs_declaration_order, module->funs_declaration_order, module->structs_table, module->funs_table);
		lexer.reset(module_source);
		parse_fun_bodies(lexer, arena, module->funs_declaration_order, module->structs_table, module->funs_table);
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
