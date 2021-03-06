#include "./parser.h"

#include "../../util/store/ArenaArrayBuilders.h"
#include "../../util/store/ListBuilder.h"
#include "./Lexer.h"
#include "./parse_expr.h"
#include "./parse_type.h"

namespace {
	// Assumes we've already taken a ' ' to indicate we want to parse at least one type parameter.
	Slice<TypeParameterAst> parse_type_parameters(Lexer& lexer, Arena& arena) {
		MaxSizeVector<4, TypeParameterAst> type_parameters;
		uint index = 0;
		do {
			lexer.take('?');
			type_parameters.push({ lexer.take_type_name(), index });
			++index;
		} while (lexer.try_take(' '));
		return to_arena(type_parameters, arena);
	}

	struct TypeParametersAndSpecs {
		Slice<TypeParameterAst> type_parameters;
		Slice<SpecUseAst> specs;
	};
	TypeParametersAndSpecs parse_type_parameters_and_spec_uses(Lexer& lexer, Arena& arena) {
		MaxSizeVector<4, TypeParameterAst> type_parameters;
		MaxSizeVector<4, SpecUseAst> spec_uses;
		uint index = 0;
		while (true) {
			if (!lexer.try_take(' ')) break;
			if (!lexer.try_take('?')) {
				do {
					StringSlice name = lexer.take_spec_name();
					spec_uses.push({ name, parse_type_arguments(lexer, arena) });
				} while (lexer.try_take(' '));
				break;
			}
			type_parameters.push({ lexer.take_type_name(), index });
			++index;
		}
		return { to_arena(type_parameters, arena), to_arena(spec_uses, arena) };
	}

	Slice<ParameterAst> parse_parameters(Lexer& lexer, Arena& arena) {
		if (!lexer.try_take('('))
			return {};
		MaxSizeVector<4, ParameterAst> parameters;
		while (true) {
			if (lexer.try_take(')')) todo(); //error: Don't write `()`

			StringSlice name = lexer.take_value_name();
			lexer.take(' ');
			TypeAst type = parse_type(lexer, arena);
			parameters.push({ name, type });
			if (lexer.try_take(')')) break;
			lexer.take(',');
			lexer.take(' ');
		}
		return to_arena(parameters, arena);
	}

	FunSignatureAst parse_signature(Lexer& lexer, Arena& arena, StringSlice name, Option<ArenaString> comment) {
		Option<Effect> effect = lexer.try_take_effect();
		TypeAst return_type = parse_type(lexer, arena);
		Slice<ParameterAst> parameters = parse_parameters(lexer, arena);
		TypeParametersAndSpecs tp = parse_type_parameters_and_spec_uses(lexer, arena);
		return { comment, name, effect, return_type, parameters, tp.type_parameters, tp.specs };
	}

	Slice<StructFieldAst> parse_struct_fields(Lexer& lexer, Arena& arena) {
		if (!lexer.try_take_indent())
			return {};

		MaxSizeVector<4, StructFieldAst> b;
		do {
			Option<ArenaString> comment = lexer.try_take_comment(arena);
			StringSlice name = lexer.take_value_name();
			lexer.take(' ');
			TypeAst type = parse_type(lexer, arena);
			b.push({ comment, name, type });
		} while (lexer.take_newline_or_dedent() == NewlineOrDedent::Newline);
		return to_arena(b, arena);
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
		Slice<TypeParameterAst> type_parameters = lexer.try_take(' ') ? parse_type_parameters(lexer, arena) : Slice<TypeParameterAst>{};
		lexer.take_indent();
		MaxSizeVector<4, FunSignatureAst> sigs;
		do {
			Option<ArenaString> sig_comment = lexer.try_take_comment(arena);
			StringSlice sig_name = lexer.take_value_name();
			lexer.take(' ');
			sigs.push(parse_signature(lexer, arena, sig_name, sig_comment));
		} while (lexer.take_newline_or_dedent() == NewlineOrDedent::Newline);
		return SpecDeclarationAst { comment, lexer.range(start), is_public, name, type_parameters, to_arena(sigs, arena) };
	}

	void parse_struct_or_fun(Lexer& lexer, Arena& arena, bool is_public, const char* start, Option<ArenaString> comment,
		ListBuilder<StringSlice>& includes, ListBuilder<StructDeclarationAst>& structs, ListBuilder<FunDeclarationAst>& funs) {
		bool c = lexer.try_take('c');
		if (c) lexer.take(' ');

		Lexer::ValueOrTypeName name = lexer.take_value_or_type_name();
		if (name.is_value) {
			lexer.take(' ');
			if (name.name == INCLUDE) {
				// is_public is irrelevant for these
				includes.add(lexer.take_cpp_include(), arena);
			} else {
				FunSignatureAst signature = parse_signature(lexer, arena, name.name, comment);
				FunBodyAst body = c ? FunBodyAst { lexer.take_indented_string(arena) } : FunBodyAst { parse_body(lexer, arena) };
				funs.add({ is_public, signature, body }, arena);
			}
		} else {
			bool copy = false;
			Slice<TypeParameterAst> type_parameters;
			if (lexer.try_take(' ')) {
				copy = lexer.try_take_copy_keyword();
				if (!copy || lexer.try_take(' '))
					type_parameters = parse_type_parameters(lexer, arena);
			}
			StructBodyAst body = c ? StructBodyAst { parse_cpp_struct_body(lexer) } : StructBodyAst { parse_struct_fields(lexer, arena) };
			structs.add({ comment, lexer.range(start), is_public, name.name, type_parameters, copy, body }, arena);
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

	Slice<ImportAst> parse_imports(Lexer& lexer, Arena& arena, PathCache& path_cache) {
		MaxSizeVector<4, ImportAst> b;
		do {
			b.push(parse_single_import(lexer, path_cache));
		} while (lexer.try_take(' '));
		return to_arena(b, arena);
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

	ListBuilder<StringSlice> includes;
	ListBuilder<SpecDeclarationAst> specs;
	ListBuilder<StructDeclarationAst> structs;
	ListBuilder<FunDeclarationAst> funs;

	bool is_public = true;
	while (true) {
		lexer.skip_blank_lines();
		if (lexer.try_take('\0'))
			break;
		if (lexer.try_take_private_nl()) {
			if (!is_public) todo(); // we're already private, so this is unnecessary
			is_public = false;
			lexer.skip_blank_lines();
		}

		Option<ArenaString> comment = lexer.try_take_comment(arena);
		const char* start = lexer.at();
		if (lexer.try_take('$'))
			specs.add(parse_spec(lexer, arena, is_public, comment), arena);
		else
			parse_struct_or_fun(lexer, arena, is_public, start, comment, includes, structs, funs);
	}

	ast.includes = includes.finish();
	ast.specs = specs.finish();
	ast.structs = structs.finish();
	ast.funs = funs.finish();
}
