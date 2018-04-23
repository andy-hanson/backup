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
		Arena::SmallArrayBuilder<ParameterAst> parameters = arena.small_array_builder<ParameterAst>();
		lexer.take('(');
		if (!lexer.try_take(')')) {
			while (true) {
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
		}
		return parameters.finish();
	}

	FunSignatureAst parse_signature_ast(Lexer& lexer, Arena& arena, StringSlice name) {
		Option<Effect> effect = lexer.try_take_effect();
		TypeAst return_type = parse_type_ast(lexer, arena);
		Arr<ParameterAst> parameters = parse_parameter_asts(lexer, arena);
		std::pair<Arr<TypeParameterAst>, Arr<SpecUseAst>> tp = parse_type_parameter_and_spec_use_asts(lexer, arena);
		return { name, effect, return_type, parameters, tp.first, tp.second };
	}

	Arr<StructFieldAst> parse_struct_field_asts(Lexer& lexer, Arena& arena) {
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
		lexer.take_indent();
		StringSlice name = lexer.take_cpp_type_name();
		lexer.take_dedent();
		return StructBodyAst(name);
	}

	const StringSlice INCLUDE { "include" };

	DeclarationAst parse_single_declaration(Lexer& lexer, Arena& arena) {
		const char* start = lexer.at();
		if (lexer.try_take('$')) {
			StringSlice name = lexer.take_type_name();
			Arr<TypeParameterAst> type_parameters = lexer.try_take(' ') ? parse_type_parameter_asts(lexer, arena) : Arr<TypeParameterAst>{};
			lexer.take_indent();
			Arena::SmallArrayBuilder<FunSignatureAst> sigs = arena.small_array_builder<FunSignatureAst>();
			do {
				StringSlice sig_name = lexer.take_value_name();
				lexer.take(' ');
				sigs.add(parse_signature_ast(lexer, arena, sig_name));
			} while (lexer.take_newline_or_dedent() == NewlineOrDedent::Newline);
			return SpecDeclarationAst { name, type_parameters, sigs.finish() };
		} else {
			bool c = lexer.try_take('c');
			if (c) lexer.take(' ');

			Lexer::ValueOrTypeName vt = lexer.take_value_or_type_name();
			if (vt.is_value) {
				lexer.take(' ');
				if (vt.name == INCLUDE) {
					return lexer.take_cpp_include();
				} else {
					FunSignatureAst signature = parse_signature_ast(lexer, arena, vt.name);
					FunBodyAst body = c ? FunBodyAst { lexer.take_indented_string(arena) } : FunBodyAst { parse_body_ast(lexer, arena) };
					return FunDeclarationAst { signature, body };
				}
			} else {
				bool copy = false;
				Arr<TypeParameterAst> type_parameters;
				if (lexer.try_take(' ')) {
					copy = lexer.try_take_copy_keyword();
					if (!copy || lexer.try_take(' '))
						type_parameters = parse_type_parameter_asts(lexer, arena);
				}
				StructBodyAst body = c ? parse_cpp_struct_body(lexer) : parse_struct_field_asts(lexer, arena);
				return StructDeclarationAst { lexer.range(start), vt.name, type_parameters, copy, body };
			}
		}
	}
}

Vec<DeclarationAst> parse_file(const StringSlice& file_content, Arena& arena) {
	Lexer::validate_file(file_content);

	Vec<DeclarationAst> declarations;
	Lexer lexer { file_content };
	while (true) {
		lexer.skip_blank_lines();
		if (lexer.try_take('\0'))
			break;
		declarations.push(parse_single_declaration(lexer, arena));
	}
	return declarations;
}
