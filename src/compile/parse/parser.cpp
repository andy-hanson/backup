#include "./parser.h"

#include "../../util/ArenaArrayBuilders.h"
#include "./Lexer.h"
#include "./parse_expr.h"
#include "./parse_type.h"

namespace {
	// Assumes we've already taken a ' ' to indicate we want to parse at least one type parameter.
	Slice<TypeParameterAst> parse_type_parameter_asts(Lexer& lexer, Arena& arena) {
		SmallArrayBuilder<TypeParameterAst> type_parameters;
		uint index = 0;
		do {
			lexer.take('?');
			type_parameters.add({ lexer.take_type_name(), index });
			++index;
		} while (lexer.try_take(' '));
		return type_parameters.finish(arena);
	}


	struct TypeParametersAndSpecs {
		Slice<TypeParameterAst> type_parameters;
		Slice<SpecUseAst> specs;
	};
	TypeParametersAndSpecs parse_type_parameter_and_spec_use_asts(Lexer& lexer, Arena& arena) {
		SmallArrayBuilder<TypeParameterAst> type_parameters;
		SmallArrayBuilder<SpecUseAst> spec_uses;
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
		return { type_parameters.finish(arena), spec_uses.finish(arena) };
	}

	Slice<ParameterAst> parse_parameter_asts(Lexer& lexer, Arena& arena) {
		if (!lexer.try_take('(')) {
			return {};
		}
		SmallArrayBuilder<ParameterAst> parameters;
		lexer.take('(');
		while (true) {
			if (lexer.try_take(')')) todo(); //error: Don't write `()`

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
		return parameters.finish(arena);
	}

	FunSignatureAst parse_signature_ast(Lexer& lexer, Arena& arena, StringSlice name, Option<ArenaString> comment) {
		Option<Effect> effect = lexer.try_take_effect();
		TypeAst return_type = parse_type_ast(lexer, arena);
		Slice<ParameterAst> parameters = parse_parameter_asts(lexer, arena);
		TypeParametersAndSpecs tp = parse_type_parameter_and_spec_use_asts(lexer, arena);
		return { comment, name, effect, return_type, parameters, tp.type_parameters, tp.specs };
	}

	Slice<StructFieldAst> parse_struct_field_asts(Lexer& lexer, Arena& arena) {
		lexer.take_indent();
		SmallArrayBuilder<StructFieldAst> b;
		do {
			Option<ArenaString> comment = lexer.try_take_comment(arena);
			TypeAst type = parse_type_ast(lexer, arena);
			lexer.take(' ');
			b.add({ comment, type, lexer.take_value_name() });
		} while (lexer.take_newline_or_dedent() == NewlineOrDedent::Newline);
		return b.finish(arena);
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
		Slice<TypeParameterAst> type_parameters = lexer.try_take(' ') ? parse_type_parameter_asts(lexer, arena) : Slice<TypeParameterAst>{};
		lexer.take_indent();
		SmallArrayBuilder<FunSignatureAst> sigs;
		do {
			Option<ArenaString> sig_comment = lexer.try_take_comment(arena);
			StringSlice sig_name = lexer.take_value_name();
			lexer.take(' ');
			sigs.add(parse_signature_ast(lexer, arena, sig_name, sig_comment));
		} while (lexer.take_newline_or_dedent() == NewlineOrDedent::Newline);
		return SpecDeclarationAst { comment, lexer.range(start), is_public, name, type_parameters, sigs.finish(arena) };
	}

	void parse_struct_or_fun(Lexer& lexer, Arena& arena, bool is_public, const char* start, Option<ArenaString> comment,
		List<StringSlice>::Builder& includes, List<StructDeclarationAst>::Builder& structs, List<FunDeclarationAst>::Builder& funs) {
		bool c = lexer.try_take('c');
		if (c) lexer.take(' ');

		Lexer::ValueOrTypeName name = lexer.take_value_or_type_name();
		if (name.is_value) {
			lexer.take(' ');
			if (name.name == INCLUDE) {
				// is_public is irrelevant for these
				includes.add(lexer.take_cpp_include(), arena);
			} else {
				FunSignatureAst signature = parse_signature_ast(lexer, arena, name.name, comment);
				FunBodyAst body = c ? FunBodyAst { lexer.take_indented_string(arena) } : FunBodyAst { parse_body_ast(lexer, arena) };
				funs.add({ is_public, signature, body }, arena);
			}
		} else {
			bool copy = false;
			Slice<TypeParameterAst> type_parameters;
			if (lexer.try_take(' ')) {
				copy = lexer.try_take_copy_keyword();
				if (!copy || lexer.try_take(' '))
					type_parameters = parse_type_parameter_asts(lexer, arena);
			}
			StructBodyAst body = c ? StructBodyAst { parse_cpp_struct_body(lexer) } : StructBodyAst { parse_struct_field_asts(lexer, arena) };
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
		SmallArrayBuilder<ImportAst> b;
		do {
			b.add(parse_single_import(lexer, path_cache));
		} while (lexer.try_take(' '));
		return b.finish(arena);
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

	List<StringSlice>::Builder includes;
	List<SpecDeclarationAst>::Builder specs;
	List<StructDeclarationAst>::Builder structs;
	List<FunDeclarationAst>::Builder funs;

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
