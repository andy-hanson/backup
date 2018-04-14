#include <unordered_map>

#include "parser.h"

#include "Lexer.h"
#include "../util/collection_util.h"

#include "parser_internal.h"
#include "parse_expr.h"
#include "check_function_body.h"

namespace {
	Type parse_type(Lexer& lexer, Arena& arena, const StructsTable& structs_table, const DynArray<TypeParameter>& type_parameters_in_scope);

	TypeParameter parse_type_parameter(Lexer& lexer, Arena& arena, uint index) {
		StringSlice name = lexer.take_type_name();
		return { Identifier{arena.str(name)}, index };
	}

	DynArray<TypeParameter> parse_type_parameters(Lexer& lexer, Arena& arena, bool end_in_space) {
		if (!lexer.try_take('<')) return {};

		auto parameters = arena.small_array_builder<TypeParameter>();
		uint index = 0;
		do {
			parameters.add(parse_type_parameter(lexer, arena, index));
			++index;
		} while (lexer.try_take_comma_space());
		lexer.take('>');
		if (end_in_space) lexer.take(' ');
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

	void parse_struct_header(Lexer& lexer, Arena& arena, ref<const Module> containing_module, StructsDeclarationOrder& structs, StructsTable& structs_table) {
		DynArray<TypeParameter> type_parameters = parse_type_parameters(lexer, arena, /*end_in_space*/ false);
		lexer.take(' ');
		StringSlice name = lexer.take_type_name();
		uint n_fields = lexer.skip_indent_and_indented_lines();
		ref<StructDeclaration> s = arena.emplace<StructDeclaration>()(containing_module, Identifier{arena.str(name)}, type_parameters, arena.new_array<StructField>(n_fields));
		structs.push_back(s);
		bool inserted = structs_table.try_insert(s->name.str, s);
		if (!inserted) throw "todo";
	}

	void fill_struct(Lexer& lexer, StructDeclaration& s, Arena& arena, const StructsTable& structs_table) {
		assert(s.body.is_fields());
		lexer.skip_to_end_of_line();
		lexer.take_indent();
		DynArray<StructField>& fields = s.body.fields();
		fields.fill([&](uint i) -> StructField {
			Type type = parse_type(lexer, arena, structs_table, s.type_parameters);
			lexer.take(' ');
			StringSlice name = lexer.take_value_name();
			if (i == fields.size() - 1) lexer.take_dedent(); else lexer.take_newline_same_indent();
			return { type, Identifier{arena.str(name)} };
		});
	}

	void add_overload(FunsTable& funs_table, ref<Fun> f) {
		OverloadGroup& group = funs_table.get_or_create(f->name.str); // Creates a new empty group if necessary
		group.funs.push_back(f); // TODO: warn if adding same signature twice
	}

	void parse_fun_header_skip_body(Lexer& lexer, Arena& arena, ref<const Module> containing_module, FunsDeclarationOrder& funs, const StructsTable& structs_table, FunsTable& funs_table) {
		DynArray<TypeParameter> type_parameters = parse_type_parameters(lexer, arena, /*end_in_space*/ true);
		Type return_type = parse_type(lexer, arena, structs_table, type_parameters);
		lexer.take(' ');
		StringSlice fn_name = lexer.take_value_name();

		Arena::SmallArrayBuilder<Parameter> parameters = arena.small_array_builder<Parameter>();

		lexer.take('(');
		if (!lexer.try_take(')')) {
			while (true) {
				Type param_type = parse_type(lexer, arena, structs_table, type_parameters);
				lexer.take(' ');
				parameters.emplace(param_type, Identifier{arena.str(lexer.take_value_name())});

				if (lexer.try_take(')')) break;
				lexer.take(',');
				lexer.take(' ');
			}
		}

		ref<Fun> f = arena.emplace<Fun>()(containing_module, type_parameters, return_type, Identifier{arena.str(fn_name)}, parameters.finish());
		funs.push_back(f);
		add_overload(funs_table, f);

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

	void parse_type_headers(Lexer& lexer, Arena& arena, ref<const Module> containing_module, StructsDeclarationOrder& structs, StructsTable& structs_table) {
		while (true) {
			switch (lexer.try_take_top_level_keyword()) {
				case TopLevelKeyword::KwCppInclude:
					throw "todo";

				case TopLevelKeyword::KwCppStruct: {
					DynArray<TypeParameter> type_parameters = parse_type_parameters(lexer, arena, /*end_in_space*/ false);
					lexer.take(' ');
					StringSlice name = lexer.take_type_name();
					ref<StructDeclaration> s = arena.emplace<StructDeclaration>()(containing_module, Identifier { arena.str(name) }, type_parameters, parse_cpp_struct_body(lexer, arena));
					structs.push_back(s);
					bool inserted = structs_table.try_insert(s->name.str, s);
					if (!inserted) throw "todo";
					break;
				}

				case TopLevelKeyword::KwCpp: // cpp function
					lexer.skip_to_end_of_line_and_indented_lines();
					break;

				case TopLevelKeyword::KwStruct:
					parse_struct_header(lexer, arena, containing_module, structs, structs_table);
					break;

				case TopLevelKeyword::KwEof:
					return;

				case TopLevelKeyword::None: // function
					lexer.skip_to_end_of_line_and_indented_lines();
					break;
			}
		}
	}

	void parse_fun_headers_and_type_bodies(Lexer& lexer, Arena& arena, ref<const Module> containing_module, StructsDeclarationOrder& structs, FunsDeclarationOrder& funs, StructsTable& structs_table, FunsTable& funs_table) {
		auto struct_iter = structs.begin();
		auto struct_end = structs.end();

		while (true) {
			switch (lexer.try_take_top_level_keyword()) {
				case TopLevelKeyword::KwCppInclude:
					throw "todo";

				case TopLevelKeyword::KwCppStruct:
					lexer.skip_to_end_of_line_and_optional_indented_lines();
					break;

				case TopLevelKeyword ::KwCpp: // cpp function
					parse_fun_header_skip_body(lexer, arena, containing_module, funs, structs_table, funs_table);
					break;

				case TopLevelKeyword::KwStruct:
					assert(struct_iter != struct_end);
					fill_struct(lexer, **struct_iter, arena, structs_table);
					++struct_iter;
					break;

				case TopLevelKeyword::KwEof:
					return;

				case TopLevelKeyword::None: // function
					parse_fun_header_skip_body(lexer, arena, containing_module, funs, structs_table, funs_table);
					break;
			}
		}

		assert(struct_iter == struct_end);
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

	void parse_fun_bodies(Lexer& lexer, Arena& arena, FunsDeclarationOrder& funs, const StructsTable& structs_table, const FunsTable& funs_table) {
		Option<Type> bool_type = get_special_named_type(structs_table, BOOL);
		Option<Type> string_type = get_special_named_type(structs_table, STRING);
		Option<Type> void_type = get_special_named_type(structs_table, VOID);

		// We know we'll be going over the funs in the same order as before, so don't need to do map lookup to get them.
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
					throw "todo";

				case TopLevelKeyword::KwCppStruct:
					lexer.skip_to_end_of_line_and_optional_indented_lines();
					break;

				case TopLevelKeyword ::KwCpp: // cpp function
					lexer.skip_to_end_of_line();
					eat_fun().body = lexer.take_indented_string(arena);
					break;

				case TopLevelKeyword::KwStruct:
					lexer.skip_to_end_of_line_and_indented_lines();
					break;

				case TopLevelKeyword::KwEof:
					assert(fun_iter == fun_end);
					return;

				case TopLevelKeyword::None: { // function
					lexer.skip_to_end_of_line();
					lexer.take_indent();
					Fun& fun = eat_fun();
					fun.body = convert(parse_body_ast(lexer, scratch_arena, arena), arena, scratch_arena, funs_table, structs_table, fun.parameters, bool_type, string_type, void_type, fun.return_type);
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
		parse_type_headers(lexer, arena, module, module->structs_declaration_order, module->structs_table);
		lexer.reset(module_source);
		parse_fun_headers_and_type_bodies(lexer, arena, module, module->structs_declaration_order, module->funs_declaration_order, module->structs_table, module->funs_table);
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
