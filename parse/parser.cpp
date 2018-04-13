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

	DynArray<TypeParameter> parse_type_parameters(Lexer& lexer, Arena& arena) {
		if (!lexer.try_take('<')) return {};

		auto parameters = arena.small_array_builder<TypeParameter>();
		uint index = 0;
		do {
			parameters.add(parse_type_parameter(lexer, arena, index));
			++index;
		} while (lexer.try_take_comma_space());
		lexer.take('>');
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

	void parse_struct_header(Lexer& lexer, Arena& arena, Structs& structs, StructsTable& structs_table) {
		DynArray<TypeParameter> type_parameters = parse_type_parameters(lexer, arena);
		lexer.take(' ');
		StringSlice name = lexer.take_type_name();
		uint n_fields = lexer.skip_indent_and_indented_lines();
		StructDeclaration& s = structs.emplace(Identifier{arena.str(name)}, type_parameters, arena.new_array<StructField>(n_fields));
		bool inserted = structs_table.insert(StringSlice(s.name), ref<const StructDeclaration>(&s));
		if (!inserted) throw "todo";
	}

	void fill_struct(Lexer& lexer, StructDeclaration& s, Arena& arena, const StructsTable& structs_table) {
		assert(s.body.kind() == StructBody::Kind::Fields);
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

	void add_overload(FunsTable& funs_table, Fun& f) {
		OverloadGroup& group = funs_table.get_or_create(f.name); // Creates a new empty group if necessary
		group.funs.push_back(&f); // TODO: warn if adding same signature twice
	}

	Fun& parse_fun_header_skip_body(Lexer& lexer, Arena& arena, Funs& funs, const StructsTable& structs_table, FunsTable& funs_table) {
		DynArray<TypeParameter> type_parameters = parse_type_parameters(lexer, arena);
		lexer.take(' ');
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

		Fun& f = funs.emplace(type_parameters, return_type, Identifier{arena.str(fn_name)}, parameters.finish());
		add_overload(funs_table, f);

		lexer.skip_indent_and_indented_lines();
		return f;
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

	void parse_type_headers(Lexer& lexer, Arena& arena, Structs& structs, StructsTable& structs_table) {
		while (true) {
			switch (lexer.take_top_level_keyword()) {
				case TopLevelKeyword::KwCPlusPlus:
					lexer.take(' ');
					switch (lexer.take_top_level_keyword()) {
						case TopLevelKeyword::KwStruct: {
							DynArray<TypeParameter> type_parameters = parse_type_parameters(lexer, arena);
							lexer.take(' ');
							StringSlice name = lexer.take_type_name();
							StructDeclaration& c = structs.emplace(Identifier{arena.str(name)}, type_parameters, parse_cpp_struct_body(lexer, arena));
							bool inserted = structs_table.insert(StringSlice(c.name), &c);
							if (!inserted) throw "todo";
							break;
						}
						case TopLevelKeyword::KwFun:
							lexer.skip_to_end_of_line_and_indented_lines();
							break;
						case TopLevelKeyword::KwCPlusPlus:
						case TopLevelKeyword::KwEof:
							throw "todo";
					}
					break;

				case TopLevelKeyword::KwStruct:
					parse_struct_header(lexer, arena, structs, structs_table);
					break;

				case TopLevelKeyword::KwFun:
					lexer.skip_to_end_of_line_and_indented_lines();
					break;

				case TopLevelKeyword::KwEof:
					return;
			}
		}
	}

	void parse_fun_headers_and_type_bodies(Lexer& lexer, Arena& arena, Structs& structs, Funs& funs, StructsTable& structs_table, FunsTable& funs_table) {
		auto struct_iter = structs.begin();
		auto struct_end = structs.end();

		while (true) {
			switch (lexer.take_top_level_keyword()) {
				case TopLevelKeyword::KwCPlusPlus:
					lexer.take(' ');
					switch (lexer.take_top_level_keyword()) {
						case TopLevelKeyword::KwStruct:
							lexer.skip_to_end_of_line_and_optional_indented_lines();
							break;
						case TopLevelKeyword::KwFun:
							parse_fun_header_skip_body(lexer, arena, funs, structs_table, funs_table);
							break;
						case TopLevelKeyword::KwCPlusPlus:
						case TopLevelKeyword::KwEof:
							assert(false); //should have handled this last pass
					}
					break;

				case TopLevelKeyword::KwStruct:
					assert(struct_iter != struct_end);
					fill_struct(lexer, *struct_iter, arena, structs_table);
					++struct_iter;
					break;

				case TopLevelKeyword::KwFun:
					parse_fun_header_skip_body(lexer, arena, funs, structs_table, funs_table);
					break;

				case TopLevelKeyword::KwEof:
					return;
			}
		}

		assert(struct_iter == struct_end);
	}

	const StringSlice BOOL = StringSlice { "Bool" };
	const StringSlice STRING = StringSlice { "String" };

	Option<Type> get_special_named_type(const StructsTable& structs_table, StringSlice type_name) {
		return map<const ref<const StructDeclaration>&, Type>(structs_table.get(type_name))([](ref<const StructDeclaration> rf) -> Type {
			if (rf->arity()) throw "todo: Bool/String shouldn't have type parameters";
			return { Effect::Pure, { rf, {} } };
		});
	}

	void parse_fun_bodies(Lexer& lexer, Arena& arena, Funs& funs, const StructsTable& structs_table, const FunsTable& funs_table) {
		Option<Type> bool_type = get_special_named_type(structs_table, BOOL);
		Option<Type> string_type = get_special_named_type(structs_table, STRING);

		// We know we'll be going over the funs in the same order as before, so don't need to do map lookup to get them.
		auto fun_iter = funs.begin();
		auto fun_end = funs.end();
		auto eat_fun = [&fun_iter, fun_end]() -> Fun& {
			assert(fun_iter != fun_end);
			Fun& res = *fun_iter;
			++fun_iter;
			return res;
		};

		Arena scratch_arena;

		while (true) {
			switch (lexer.take_top_level_keyword()) {
				case TopLevelKeyword::KwCPlusPlus:
					lexer.take(' ');
					switch (lexer.take_top_level_keyword()) {
						case TopLevelKeyword::KwCPlusPlus:
						case TopLevelKeyword::KwEof:
							throw "todo";

						case TopLevelKeyword::KwStruct:
							lexer.skip_to_end_of_line_and_optional_indented_lines();
							break;

						case TopLevelKeyword::KwFun:
							lexer.skip_to_end_of_line();
							eat_fun().body = lexer.take_indented_string(arena);
							break;
					}
					break;

				case TopLevelKeyword::KwStruct:
					lexer.skip_to_end_of_line_and_indented_lines();
					break;

				case TopLevelKeyword::KwFun: {
					lexer.skip_to_end_of_line();
					lexer.take_indent();
					Fun& fun = eat_fun();
					fun.body = convert(parse_body_ast(lexer, scratch_arena, arena), arena, scratch_arena, funs_table, structs_table, fun.parameters, bool_type, string_type, fun.return_type);
					scratch_arena.clear();
					break;
				}

				case TopLevelKeyword::KwEof:
					assert(fun_iter == fun_end);
					return;
			}
		}
	}
}

Module parse_file(StringSlice module_source) {
	Lexer lexer { module_source };
	Module module;
	parse_type_headers(lexer, module.arena, module.structs, module.structs_table);
	lexer.reset(module_source);
	parse_fun_headers_and_type_bodies(lexer, module.arena, module.structs, module.funs, module.structs_table, module.funs_table);
	lexer.reset(module_source);
	parse_fun_bodies(lexer, module.arena, module.funs, module.structs_table, module.funs_table);
	return module;
}
