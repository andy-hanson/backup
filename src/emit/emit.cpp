#include "emit.h"

#include "../compile/model/expr.h"
#include "../util/Arena.h"
#include "../util/Writer.h"
#include "./concrete_fun.h"
#include "./emit_body.h"
#include "./emit_comment.h"
#include "./emit_type.h"
#include "./mangle.h"
#include "./Names.h"

namespace {
	void write_type_parameters(Writer& out, const Arr<TypeParameter>& type_parameters) {
		if (type_parameters.empty()) return;
		out << "template <";
		for (uint i = 0; i != type_parameters.size(); ++i) {
			if (i != 0) out << ", ";
			out << "typename " << mangle{type_parameters[i].name};
		}
		out << '>' << Writer::nl;
	}

	void emit_struct(Writer& out, const StructDeclaration& s, const Names& names) {
		emit_comment(out, s.comment);
		write_type_parameters(out, s.type_parameters);
		const StringSlice& name = names.get_name(&s);
		switch (s.body.kind()) {
			case StructBody::Kind::Nil: assert(false);
			case StructBody::Kind::Fields:
				out << "struct " << name << " {" << Writer::indent << Writer::nl;
				for (const StructField& field : s.body.fields()) {
					emit_comment(out, field.comment);
					write_type(out, field.type, names);
					out << ' ' << names.get_name(&field) << ';' << Writer::nl;
				}
				out << Writer::dedent << Writer::nl << "};" << Writer::nl << Writer::nl;
				break;
			case StructBody::Kind::CppName:
				out << "typedef " << s.body.cpp_name() << ' ' << name << ";\n\n";
				break;
		}
	}

	void write_fun_header(Writer& out, const ConcreteFun &f, const Names& names, Arena& scratch_arena) {
		emit_comment(out, f.fun_declaration->signature.comment);
		substitute_and_write_inst_struct(out, f, f.fun_declaration->signature.return_type, names, scratch_arena, f.fun_declaration->signature.effect == Effect::EOwn);
		out << ' ' << names.get_name(&f) << '(';
		bool first_param = true;
		for (const Parameter& param : f.fun_declaration->signature.parameters) {
			if (first_param)
				first_param = false;
			else
				out << ", ";
			substitute_and_write_inst_struct(out, f, param.type, names, scratch_arena, param.effect == Effect::EOwn);
			out << ' ' << mangle{param.name};
		}
		out << ')';
	}

	enum class EmitStructState { Nil, Emitted, Emitting };

	template <typename Cb>
	void each_struct_field(const StructDeclaration& s, Cb cb) {
		if (!s.body.is_fields()) return;
		for (const StructField& f : s.body.fields())
			if (f.type.is_inst_struct())
				cb(f.type.inst_struct().strukt);
	}

	void emit_structs(Writer& out, const StructsDeclarationOrder& structs, const Names& names) {
		MaxSizeMap<32, ref<const StructDeclaration>, EmitStructState, ref<const StructDeclaration>::hash> map;
		MaxSizeVector<16, ref<const StructDeclaration>> stack;

		for (const StructDeclaration& struct_in_order : structs) {
			stack.push(&struct_in_order);
			do {
				// At each step, we either pop, or mark a struct as emitting (which will be popped next time). So should terminate eventually.
				ref<const StructDeclaration> s = stack.peek();
				EmitStructState& state = map.get_or_insert_default(s);
				switch (state) {
					case EmitStructState::Emitted:
						stack.pop();
						break;
					case EmitStructState::Emitting:
						stack.pop();
						emit_struct(out, s, names);
						state = EmitStructState::Emitted;
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
		out << ";\n\n";
	}

	void emit_fun_with_body(Writer& out, ref<const ConcreteFun> f, const Names& names, const ResolvedCalls& resolved_calls, Arena& scratch_arena) {
		write_fun_header(out, f, names, scratch_arena);
		out << " {\n\t";
		// Writing the body will be slightly different each time because we map each Called in the body to a different ConcreteFun depending on 'f'.
		emit_body(out, f, names, resolved_calls, scratch_arena);
		out << "\n}\n\n";
	}

	template <typename /*ref<constConcreteFun>> => void>*/ Cb>
	void each_concrete_fun(const Module& module, const FunInstantiations& fun_instantiations, Cb cb) {
		for (const FunDeclaration& f : module.funs_declaration_order) {
			Option<const NonEmptyList<ConcreteFun>&> instantiations = fun_instantiations.get(&f);
			if (instantiations.has())
				for (const ConcreteFun& cf : instantiations.get())
					cb(&cf);
		}
	}
}

Grow<char> emit(const Arr<Module>& modules) {
	assert(!modules.empty());
	Grow<char> grow;
	Writer out { grow };
	out << "#include <assert.h>\n\n";

	Arena scratch_arena;
	EveryConcreteFun every_concrete_fun = get_every_concrete_fun(modules, scratch_arena);
	Names names = get_names(modules, every_concrete_fun.fun_instantiations, scratch_arena);

	// First, emit all structs and function headers.
	for (const Module& module : modules) {
		emit_comment(out, module.comment);
		emit_structs(out, module.structs_declaration_order, names);
		each_concrete_fun(module, every_concrete_fun.fun_instantiations, [&](ref<const ConcreteFun> cf) { emit_fun_header(out, cf, names, scratch_arena); });
	}

	for (const Module& module : modules)
		each_concrete_fun(module, every_concrete_fun.fun_instantiations, [&](ref<const ConcreteFun> cf) { emit_fun_with_body(out, cf, names, every_concrete_fun.resolved_calls, scratch_arena); });

	out << "int main() { run(); }\n";

	return grow;
}
