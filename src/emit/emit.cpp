#include <unordered_map>
#include <unordered_set>

#include "emit.h"

#include "../model/expr.h"
#include "../util/Alloc.h"
#include "./mangle.h"

#include "./concrete_fun.h"
#include "./emit_body.h"
#include "./emit_type.h"
#include "./Names.h"
#include "./Writer.h"

namespace {

	void write_type_parameters(Writer& out, const DynArray<TypeParameter>& type_parameters) {
		if (type_parameters.empty()) return;
		out << "template <";
		for (uint i = 0; i != type_parameters.size(); ++i) {
			if (i != 0) out << ", ";
			out << "typename " << mangle{type_parameters[i].name};
		}
		out << '>' << Writer::nl;
	}

	void emit_struct(Writer& out, const StructDeclaration& s, const Names& names) {
		write_type_parameters(out, s.type_parameters);
		const StringSlice& name = names.get_name(&s);
		switch (s.body.kind()) {
			case StructBody::Kind::Fields:
				out << "struct " << name << " {\n";
				for (const StructField& field : s.body.fields()) {
					const char TAB = '\t'; // https://youtrack.jetbrains.com/issue/CPP-12650
					out << TAB << field.type << ' ' << names.get_name(&field) << ";\n";
				}
				out << "};\n\n";
				break;
			case StructBody::Kind::CppName:
				out << "using " << name << " = " << s.body.cpp_name() << ";\n\n";
				break;
			case StructBody::Kind::CppBody:
				out << "struct " << name << " {\n\t" << indented{s.body.cpp_body()} << "\n};\n\n";
				break;
		}
	}

	void write_fun_header(Writer& out, const ConcreteFun &f, const Names& names, Arena& scratch_arena) {
		out << substitute_type_arguments(f.fun_declaration->signature.return_type, f, scratch_arena) << " " << names.get_name(&f) << '(';
		bool first_param = true;
		for (const auto& param : f.fun_declaration->signature.parameters) {
			if (first_param)
				first_param = false;
			else
				out << ", ";
			out << "const " << substitute_type_arguments(param.type, f, scratch_arena) << "& " << mangle{param.name};
		}
		out << ')';
	}

	enum class EmitStructState { Nil, Emitted, Emitting };

	template <typename Cb>
	void each_struct_field(const StructDeclaration& s, Cb cb) {
		if (!s.body.is_fields()) return;
		for (const StructField& f : s.body.fields())
			if (f.type.is_plain())
				cb(f.type.plain().inst_struct.strukt);
	}

	void emit_structs(Writer& out, const StructsDeclarationOrder& structs, const Names& names) {
		std::unordered_map<ref<const StructDeclaration>, EmitStructState> map;
		MaxSizeVector<16, ref<const StructDeclaration>> stack;

		for (const StructDeclaration& struct_in_order : structs) {
			stack.push(&struct_in_order);
			do {
				ref<const StructDeclaration> s = stack.peek();
				EmitStructState& state = map[s];
				switch (state) {
					case EmitStructState::Emitted:
						break;
					case EmitStructState::Emitting:
						emit_struct(out, s, names);
						state = EmitStructState::Emitted;
						stack.pop();
						break;
					case EmitStructState::Nil:
						state = EmitStructState::Emitting;
						each_struct_field(*s, [&](ref<const StructDeclaration> referenced) { stack.push(referenced); });
						break;
				}
			} while (!stack.empty());
		}
	}

	void emit_fun_header(Writer& out, const ConcreteFun& f, const Names& names, Arena& scratch_arena) {
		write_fun_header(out, f, names, scratch_arena);
		out << ';';
	}

	void emit_fun_with_body(Writer& out, ref<const ConcreteFun> f, const Names& names, const ResolvedCalls& resolved_calls, Arena& scratch_arena) {
		write_fun_header(out, f, names, scratch_arena);
		out << " {\n\t";
		// Writing the body will be slightly different each time because we map each Called in the body to a different ConcreteFun depending on 'f'.
		emit_body(out, f, names, resolved_calls, scratch_arena);
		out << "\n}\n\n";
	}
}

std::string emit(const std::vector<ref<Module>>& modules) {
	Writer out;
	Arena scratch_arena;
	EveryConcreteFun every_concrete_fun = get_every_concrete_fun(modules, scratch_arena);
	Names names = get_names(modules, every_concrete_fun.fun_instantiations, scratch_arena);

	// First, emit all structs and function headers.
	for (const Module& module : modules) {
		emit_structs(out, module.structs_declaration_order, names);
		for (ref<const FunDeclaration> f : module.funs_declaration_order)
			for (const ConcreteFun& cf : every_concrete_fun.fun_instantiations.must_get(f))
				emit_fun_header(out, cf, names, scratch_arena);
	}

	for (const Module& module : modules) {
		for (ref<const FunDeclaration> f : module.funs_declaration_order) {
			for (const ConcreteFun& cf : every_concrete_fun.fun_instantiations.must_get(f))
				emit_fun_with_body(out, &cf, names, every_concrete_fun.resolved_calls, scratch_arena);
		}
	}

	return out.finish();
}
