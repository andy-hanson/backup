#include "./parser.h"

#include "./Lexer.h"
#include "./parse_expr.h"
#include "./parse_type.h"

namespace {
	DynArray<TypeParameterAst> parse_type_parameter_asts(Lexer& lexer, Arena& arena) {
		lexer.skip_blank_lines();
		if (!lexer.try_take('<')) return {};

		Arena::SmallArrayBuilder<TypeParameterAst> parameters = arena.small_array_builder<TypeParameterAst>();
		uint index = 0;
		do {
			parameters.add({ lexer.take_type_name(), index });
			++index;
		} while (lexer.try_take_comma_space());
		lexer.take('>');
		lexer.take(' ');
		return parameters.finish();
	}

	DynArray<ParameterAst> parse_parameter_asts(Lexer& lexer, Arena& arena) {
		Arena::SmallArrayBuilder<ParameterAst> parameters = arena.small_array_builder<ParameterAst>();
		lexer.take('(');
		if (!lexer.try_take(')')) {
			while (true) {
				TypeAst type = parse_type_ast(lexer, arena);
				lexer.take(' ');
				parameters.add({ type, lexer.take_value_name() });
				if (lexer.try_take(')')) break;
				lexer.take(',');
				lexer.take(' ');
			}
		}
		return parameters.finish();
	}

	DynArray<SpecUseAst> parse_spec_use_asts(Lexer& lexer, Arena& arena) {
		if (!lexer.try_take(' ')) return {};
		Arena::SmallArrayBuilder<SpecUseAst> spec_uses = arena.small_array_builder<SpecUseAst>();
		do {
			StringSlice name = lexer.take_spec_name();
			spec_uses.add({ name, parse_type_argument_asts(lexer, arena) });
		} while (lexer.try_take(' '));
		return spec_uses.finish();
	}

	FunSignatureAst parse_signature_ast(Lexer& lexer, Arena& arena, DynArray<TypeParameterAst> type_parameters) {
		TypeAst return_type = parse_type_ast(lexer, arena);
		lexer.take(' ');
		StringSlice name = lexer.take_value_name();
		DynArray<ParameterAst> parameters = parse_parameter_asts(lexer, arena);
		DynArray<SpecUseAst> spec_uses = parse_spec_use_asts(lexer, arena);
		return { type_parameters, return_type, name, parameters, spec_uses };
	}

	DynArray<StructFieldAst> parse_struct_field_asts(Lexer& lexer, Arena& arena) {
		lexer.take_indent();
		Arena::SmallArrayBuilder<StructFieldAst> b = arena.small_array_builder<StructFieldAst>();
		do {
			TypeAst type = parse_type_ast(lexer, arena);
			lexer.take(' ');
			b.add({ type, lexer.take_value_name() });
		} while (lexer.take_newline_or_dedent() == NewlineOrDedent::Newline);
		return b.finish();
	}

	StructBodyAst parse_cpp_struct_body(Lexer& lexer) {
		lexer.take(' ');
		lexer.take('=');
		lexer.take(' ');
		return StructBodyAst(lexer.take_cpp_type_name());
	}
}

Vec<DeclarationAst> parse_file(const StringSlice& file_content, Arena& arena) {
	Lexer::validate_file(file_content);

	Vec<DeclarationAst> declarations;
	Lexer lexer { file_content };
	while (true) {
		const char* start = lexer.at();
		DynArray<TypeParameterAst> type_parameters = parse_type_parameter_asts(lexer, arena);
		TopLevelKeyword kw = lexer.try_take_top_level_keyword();
		switch (kw) {
			case TopLevelKeyword::KwCppInclude:
				lexer.take(' ');
				declarations.push(lexer.take_cpp_include());
				break;

			case TopLevelKeyword::KwCppStruct:
			case TopLevelKeyword::KwStruct: {
				lexer.take(' ');
				StringSlice name = lexer.take_type_name();
				StructBodyAst body = kw == TopLevelKeyword::KwStruct ? parse_struct_field_asts(lexer, arena) : parse_cpp_struct_body(lexer);
				declarations.push(StructDeclarationAst { lexer.range(start), type_parameters, name, body });
				break;
			}

			case TopLevelKeyword::KwSpec: {
				lexer.take(' ');
				StringSlice name = lexer.take_spec_name();
				lexer.take_indent();
				Arena::SmallArrayBuilder<FunSignatureAst> sigs = arena.small_array_builder<FunSignatureAst>();
				do {
					sigs.add(parse_signature_ast(lexer, arena, parse_type_parameter_asts(lexer, arena)));
				} while (lexer.take_newline_or_dedent() == NewlineOrDedent::Newline);
				declarations.push(SpecDeclarationAst { type_parameters, name, sigs.finish() });
				break;
			}

			case TopLevelKeyword::KwCpp:
			case TopLevelKeyword::None: {
				FunSignatureAst signature = parse_signature_ast(lexer, arena, type_parameters);
				FunBodyAst body = kw == TopLevelKeyword::KwCpp ? FunBodyAst{lexer.take_indented_string(arena)} : FunBodyAst{parse_body_ast(lexer, arena)};
				declarations.push(FunDeclarationAst { signature, body });
				break;
			}

			case TopLevelKeyword::KwEof:
				if (type_parameters.size())
					throw ParseDiagnostic { lexer.range(start), ParseDiag::Kind::TrailingTypeParametersAtEndOfFile };
				return declarations;
		}
	}
}
