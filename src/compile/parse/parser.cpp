#include "./parser.h"

#include "./Lexer.h"
#include "./parse_expr.h"
#include "./parse_type.h"

namespace {
	// Assumes we've already taken a ' ' to indicate we want to parse at least one type parameter.
	Arr<TypeParameterAst> parse_type_parameter_asts(Lexer& lexer, Arena& arena) {
		Arena::SmallArrayBuilder<TypeParameterAst> type_parameters = arena.small_array_builder<TypeParameterAst>();
		uint index = 0;
		do {
			lexer.take('?');
			type_parameters.add({ lexer.take_type_name(), index });
			++index;
		} while (lexer.try_take(' '));
		return type_parameters.finish();
	}


	std::pair<Arr<TypeParameterAst>, Arr<SpecUseAst>> parse_type_parameter_and_spec_use_asts(Lexer& lexer, Arena& arena) {
		Arena::SmallArrayBuilder<TypeParameterAst> type_parameters = arena.small_array_builder<TypeParameterAst>();
		Arena::SmallArrayBuilder<SpecUseAst> spec_uses = arena.small_array_builder<SpecUseAst>();
		uint index = 0;
		while (true) {
			if (!lexer.try_take(' ')) break;
			if (!lexer.try_take('?')) {
				do {
					StringSlice name = lexer.take_spec_name();
					spec_uses.add({ name, parse_type_argument_asts(lexer, arena) });
				} while (lexer.try_take(' '));
				break;
			}
			type_parameters.add({ lexer.take_type_name(), index });
			++index;
		}
		return { type_parameters.finish(), spec_uses.finish() };
	}

	Arr<ParameterAst> parse_parameter_asts(Lexer& lexer, Arena& arena) {
		if (!lexer.try_take('(')) {
			return {};
		}
		Arena::SmallArrayBuilder<ParameterAst> parameters = arena.small_array_builder<ParameterAst>();
		lexer.take('(');
		while (true) {
			if (lexer.try_take(')')) throw "todo"; //error: Don't write `()`

			bool from = false;
			if (lexer.try_take_from_keyword()) {
				lexer.take(' ');
				from = true;
			}
			Option<Effect> effect = lexer.try_take_effect();
			TypeAst type = parse_type_ast(lexer, arena);
			lexer.take(' ');
			parameters.add({ from, effect, type, lexer.take_value_name() });
			if (lexer.try_take(')')) break;
			lexer.take(',');
			lexer.take(' ');
		}
		return parameters.finish();
	}

	FunSignatureAst parse_signature_ast(Lexer& lexer, Arena& arena, StringSlice name, Option<ArenaString> comment) {
		Option<Effect> effect = lexer.try_take_effect();
		TypeAst return_type = parse_type_ast(lexer, arena);
		Arr<ParameterAst> parameters = parse_parameter_asts(lexer, arena);
		std::pair<Arr<TypeParameterAst>, Arr<SpecUseAst>> tp = parse_type_parameter_and_spec_use_asts(lexer, arena);
		return { comment, name, effect, return_type, parameters, tp.first, tp.second };
	}

	Arr<StructFieldAst> parse_struct_field_asts(Lexer& lexer, Arena& arena) {
		lexer.take_indent();
		Arena::SmallArrayBuilder<StructFieldAst> b = arena.small_array_builder<StructFieldAst>();
		do {
			Option<ArenaString> comment = lexer.try_take_comment(arena);
			TypeAst type = parse_type_ast(lexer, arena);
			lexer.take(' ');
			b.add({ comment, type, lexer.take_value_name() });
		} while (lexer.take_newline_or_dedent() == NewlineOrDedent::Newline);
		return b.finish();
	}

	StringSlice parse_cpp_struct_body(Lexer& lexer) {
		lexer.take_indent();
		StringSlice name = lexer.take_cpp_type_name();
		lexer.take_dedent();
		return name;
	}

	const StringSlice INCLUDE { "include" };

	SpecDeclarationAst parse_spec(Lexer& lexer, Arena& arena, bool is_public, Option<ArenaString> comment) {
		const char* start = lexer.at();
		StringSlice name = lexer.take_type_name();
		Arr<TypeParameterAst> type_parameters = lexer.try_take(' ') ? parse_type_parameter_asts(lexer, arena) : Arr<TypeParameterAst>{};
		lexer.take_indent();
		Arena::SmallArrayBuilder<FunSignatureAst> sigs = arena.small_array_builder<FunSignatureAst>();
		do {
			Option<ArenaString> sig_comment = lexer.try_take_comment(arena);
			StringSlice sig_name = lexer.take_value_name();
			lexer.take(' ');
			sigs.add(parse_signature_ast(lexer, arena, sig_name, sig_comment));
		} while (lexer.take_newline_or_dedent() == NewlineOrDedent::Newline);
		return SpecDeclarationAst { comment, lexer.range(start), is_public, name, type_parameters, sigs.finish() };
	}

	void parse_struct_or_fun(FileAst& ast, Lexer& lexer, Arena& arena, bool is_public, const char* start, Option<ArenaString> comment) {
		bool c = lexer.try_take('c');
		if (c) lexer.take(' ');

		Lexer::ValueOrTypeName name = lexer.take_value_or_type_name();
		if (name.is_value) {
			lexer.take(' ');
			if (name.name == INCLUDE) {
				// is_public is irrelevant for these
				ast.includes.push(lexer.take_cpp_include());
			} else {
				FunSignatureAst signature = parse_signature_ast(lexer, arena, name.name, comment);
				FunBodyAst body = c ? FunBodyAst { lexer.take_indented_string(arena) } : FunBodyAst { parse_body_ast(lexer, arena) };
				ast.funs.push({ is_public, signature, body });
			}
		} else {
			bool copy = false;
			Arr<TypeParameterAst> type_parameters;
			if (lexer.try_take(' ')) {
				copy = lexer.try_take_copy_keyword();
				if (!copy || lexer.try_take(' '))
					type_parameters = parse_type_parameter_asts(lexer, arena);
			}
			StructBodyAst body = c ? StructBodyAst { parse_cpp_struct_body(lexer) } : StructBodyAst { parse_struct_field_asts(lexer, arena) };
			ast.structs.push({ comment, lexer.range(start), is_public, name.name, type_parameters, copy, body });
		}
	}

	ImportAst parse_single_import(Lexer& lexer, PathCache& path_cache) {
		// Note: 1 dot = 0 parents
		const char* start = lexer.at();
		uint n_dots = 0;
		while (lexer.try_take('.')) ++n_dots;

		Path path = path_cache.from_part_slice(lexer.take_value_name());
		while (lexer.try_take('.'))
			path = path_cache.resolve(path, lexer.take_value_name());
		return { lexer.range(start), n_dots == 0 ? Option<uint>{} : Option<uint>{ n_dots - 1 }, path };
	}

	Arr<ImportAst> parse_imports(Lexer& lexer, Arena& arena, PathCache& path_cache) {
		Arena::SmallArrayBuilder<ImportAst> b = arena.small_array_builder<ImportAst>();
		do {
			b.add(parse_single_import(lexer, path_cache));
		} while (lexer.try_take(' '));
		return b.finish();
	}
}

void parse_file(FileAst& ast, PathCache& path_cache, Arena& arena) {
	Lexer::validate_file(ast.source);
	Lexer lexer { ast.source };

	ast.comment = lexer.try_take_comment(arena);

	if (lexer.try_take_import_space()) {
		ast.imports = parse_imports(lexer, arena, path_cache);
		lexer.take('\n');
	}

	bool is_public = true;
	while (true) {
		lexer.skip_blank_lines();
		if (lexer.try_take('\0'))
			break;
		if (lexer.try_take_private_nl()) {
			if (is_public) throw "todo: duplicate private";
			is_public = false;
			lexer.skip_blank_lines();
		}

		Option<ArenaString> comment = lexer.try_take_comment(arena);
		const char* start = lexer.at();
		if (lexer.try_take('$'))
			ast.specs.push(parse_spec(lexer, arena, is_public, comment));
		else
			parse_struct_or_fun(ast, lexer, arena, is_public, start, comment);
	}
}
