#include <unordered_map>
#include <unordered_set>

#include "emit.h"
#include "../util/Alloc.h"
#include "../util/Map.h"
#include "./mangle.h"

#include "Writer.h"

namespace {
	class OverloadNames {
		Arena arena;
		Map<ref<const StructDeclaration>, ArenaString> struct_names;
		Map<ref<const StructField>, ArenaString> field_names;
		Map<ref<const Fun>, ArenaString> fun_names;
		friend OverloadNames get_overload_names(const std::vector<ref<Module>>& modules);
	public:
		StringSlice get_name(ref<const StructDeclaration> s) const {
			return struct_names.get(s);
		}
		StringSlice get_name(ref<const StructField> f) const {
			return field_names.get(f);
		}
		StringSlice get_name(ref<const Fun> f) const {
			return fun_names.get(f);
		}
	};

	struct Ctx {
		Writer& out;
		const OverloadNames& overload_names;
	};

	Writer& operator<<(Writer& out, const Type& t);

	Writer& operator<<(Writer& out, const DynArray<Type>& type_arguments) {
		if (type_arguments.empty()) return out;
		out << '<';
		for (uint i = 0; i != type_arguments.size(); ++i) {
			if (i != 0) out << ", ";
			out << type_arguments[i];
		}
		return out << '>';
	}

	Writer& operator<<(Writer& out, const InstStruct& i) {
		return out << mangle{i.strukt->name} << i.type_arguments;
	}

	Writer& operator<<(Writer& out, const Type& t) {
		return t.is_parameter() ? out << mangle{t.param()->name} : out << t.plain().inst_struct;
	}

	void write_type_parameters(Writer& out, const DynArray<TypeParameter>& type_parameters) {
		if (type_parameters.empty()) return;
		out << "template <";
		for (uint i = 0; i != type_parameters.size(); ++i) {
			if (i != 0) out << ", ";
			out << "typename " << mangle{type_parameters[i].name};
		}
		out << '>' << Writer::nl;
	}

	void operator<<(Ctx& ctx, const StructDeclaration& s) {
		write_type_parameters(ctx.out, s.type_parameters);
		const StringSlice& name = ctx.overload_names.get_name(&s);
		switch (s.body.kind()) {
			case StructBody::Kind::Fields:
				ctx.out << "struct " << name << " {\n";
				for (const StructField& field : s.body.fields()) {
					const char TAB = '\t'; // https://youtrack.jetbrains.com/issue/CPP-12650
					ctx.out << TAB << field.type << ' ' << ctx.overload_names.get_name(&field) << ";\n";
				}
				ctx.out << "};\n\n";
				break;
			case StructBody::Kind::CppName:
				ctx.out << "using " << name << " = " << s.body.cpp_name() << ";\n\n";
				break;
			case StructBody::Kind::CppBody:
				ctx.out << "struct " << name << " {\n\t" << indented{s.body.cpp_body()} << "\n};\n\n";
				break;
		}
	}

	bool needs_parens_before_dot(const Expression& e) {
		switch (e.kind()) {
			case Expression::Kind::LocalReference:
			case Expression::Kind::ParameterReference:
			case Expression::Kind::StructFieldAccess:
			case Expression::Kind::Call:
			case Expression::Kind::StructCreate:
			case Expression::Kind::StringLiteral:
				return false;
			case Expression::Kind::Let:
			case Expression::Kind::Seq:
			case Expression::Kind::When:
				return true;
			case Expression::Kind::Nil:
				assert(false);
		}
	}

	void write_string_literal(Writer& out, const StringSlice& slice) {
		out << '"';
		for (char c : slice) {
			switch (c) {
				case '\n':
					out << "\\n";
					break;
				case '"':
					out << "\\\"";
					break;
				default:
					out << c;
					break;
			}
		}
		out << '"';
	}

	Ctx& operator<<(Ctx& ctx, const Expression& e) {
		Writer& out = ctx.out;
		switch (e.kind()) {
			case Expression::Kind::ParameterReference:
				out << mangle{e.parameter()->name};
				break;
			case Expression::Kind::LocalReference:
				out << mangle{e.local_reference()->name};
				break;
			case Expression::Kind::StructFieldAccess: {
				const StructFieldAccess& sa = e.struct_field_access();
				bool p = needs_parens_before_dot(*sa.target);
				if (p) out << '(';
				ctx << *sa.target;
				if (p) out << ')';
				out << '.' << ctx.overload_names.get_name(sa.field);
				break;
			}
			case Expression::Kind::Let:
				throw "todo";
			case Expression::Kind::Seq:
				throw "todo";
			case Expression::Kind::Call: {
				const Call& c = e.call();
				out << ctx.overload_names.get_name(c.called.fun) << c.called.type_arguments << "(";
				for (uint i = 0;  i != c.arguments.size(); ++i) {
					ctx << c.arguments[i];
					ctx.out << (i == c.arguments.size() - 1 ? ")" : ", ");
				}
				break;
			}
			case Expression::Kind::StructCreate: {
				// StructName { arg1, arg2 }
				const StructCreate &sc = e.struct_create();
				out << sc.inst_struct;
				out << " { ";
				for (uint i = 0; i != sc.arguments.size(); ++i) {
					ctx << sc.arguments[i];
					ctx.out << (i == sc.arguments.size() - 1 ? " }" : ", ");
				}
				break;
			}
			case Expression::Kind::StringLiteral:
				write_string_literal(out, e.string_literal());
				break;
			case Expression::Kind::When:
				throw "todo";
			case Expression::Kind::Nil:
				assert(false);
		}
		return ctx;
	}

	struct statement { const Expression& e; };
	struct return_statement { const Expression& e; };
	Ctx& write_statement(Ctx& ctx, const Expression& e, bool should_return);
	Ctx& operator<<(Ctx& ctx, statement s) { return write_statement(ctx, s.e, false); }
	Ctx& operator<<(Ctx& ctx, return_statement s) { return write_statement(ctx, s.e, true); }

	Ctx& write_statement(Ctx& ctx, const Expression& e, bool should_return) {
		switch (e.kind()) {
			case Expression::Kind::Let: {
				const Let &l = e.let();
				ctx.out << l.type << ' ' << mangle{l.name} << " = ";
				ctx << l.init;
				ctx.out << ';' << Writer::nl;
				ctx << return_statement{l.then};
				break;
			}
			case Expression::Kind::Seq: {
				const Seq& seq = e.seq();
				ctx << statement{seq.first};
				ctx.out << Writer::nl;
				ctx << return_statement{seq.then};
				break;
			}
			case Expression::Kind::When: {
				const When &w = e.when();
				for (uint i = 0; i != w.cases.size(); ++i) {
					const Case &c = w.cases[i];
					if (i != 0) ctx.out << "} else ";
					ctx.out << "if (";
					ctx << c.cond;
					ctx.out << ") {" << Writer::indent << Writer::nl;
					ctx << return_statement{c.then};
					ctx.out << Writer::dedent << Writer::nl;
				}
				ctx.out << "} else {" << Writer::indent << Writer::nl;
				ctx << return_statement{w.elze};
				ctx.out << Writer::dedent << Writer::nl << '}';
				break;
			}
			case Expression::Kind::ParameterReference:
			case Expression::Kind::LocalReference:
			case Expression::Kind::StructFieldAccess:
			case Expression::Kind::Call:
			case Expression::Kind::StructCreate:
			case Expression::Kind::StringLiteral:
			case Expression::Kind::Nil:
				if (should_return) ctx.out << "return ";
				ctx << e;
				ctx.out << ';';
		}
		return ctx;
	}

	void write_body(Ctx ctx, const AnyBody& body) {
		switch (body.kind()) {
			case AnyBody::Kind::Expression:
				ctx.out << Writer::indent;
				ctx << return_statement{body.expression()};
				ctx.out << Writer::dedent;
				break;
			case AnyBody::Kind::CppSource:
				ctx.out << indented{body.cpp_source()};
				break;
			case AnyBody::Kind::Nil:
				assert(false);
		}
	}

	void write_fun_header(Ctx &ctx, const Fun &f) {
		ctx.out << f.return_type << " " << ctx.overload_names.get_name(&f) << '(';
		bool first_param = true;
		for (const auto& param : f.parameters) {
			if (first_param)
				first_param = false;
			else
				ctx.out << ", ";
			ctx.out << "const " << param.type << "& " << mangle{param.name};
		}
		ctx.out << ')';
	}

	void write_fun(Ctx& ctx, const Fun& f) {
		write_fun_header(ctx, f);
		ctx.out << " {\n\t";
		write_body(ctx, f.body);
		ctx.out << "\n}\n\n";
	}

	enum class EmitState { Nil, Emitted, Emitting };

	template <typename Cb>
	void each_struct_field(const StructDeclaration& s, Cb cb) {
		if (!s.body.is_fields()) return;
		for (const StructField& f : s.body.fields())
			if (f.type.is_plain())
				cb(f.type.plain().inst_struct.strukt);
	}

	void emit_structs(Ctx& ctx, const StructsDeclarationOrder& structs) {
		std::unordered_map<ref<const StructDeclaration>, EmitState> map;
		MaxSizeVector<16, ref<const StructDeclaration>> stack;

		for (const StructDeclaration& struct_in_order : structs) {
			stack.push(&struct_in_order);
			do {
				ref<const StructDeclaration> s = stack.peek();
				EmitState& state = map[s];
				switch (state) {
					case EmitState::Emitted:
						break;
					case EmitState::Emitting:
						ctx << s;
						state = EmitState::Emitted;
						stack.pop();
						break;
					case EmitState::Nil:
						state = EmitState::Emitting;
						each_struct_field(*s, [&](ref<const StructDeclaration> referenced) { stack.push(referenced); });
						break;
				}
			} while (!stack.empty());
		}
	}

	template <typename Cb>
	void each_dependent_fun(const Expression& body, Cb cb) {
		MaxSizeVector<16, ref<const Expression>> stack;
		stack.push(&body);
		do {
			const Expression& e = *stack.pop_and_return();
			switch (e.kind()) {
				case Expression::Kind::ParameterReference:
				case Expression::Kind::LocalReference:
				case Expression::Kind::StructFieldAccess:
				case Expression::Kind::StringLiteral:
					break;
				case Expression::Kind::Let: {
					const Let& l = e.let();
					stack.push(&l.init);
					stack.push(&l.then);
					break;
				}
				case Expression::Kind::Seq: {
					const Seq& s = e.seq();
					stack.push(&s.first);
					stack.push(&s.then);
					break;
				}
				case Expression::Kind::Call: {
					const Call& c = e.call();
					cb(c.called.fun);
					for (const Expression& arg : c.arguments)
						stack.push(&arg);
					break;
				}
				case Expression::Kind::StructCreate:
					for (const Expression& arg : e.struct_create().arguments)
						stack.push(&arg);
					break;
				case Expression::Kind::When: {
					const When& when = e.when();
					for (const Case& c : when.cases) {
						stack.push(&c.cond);
						stack.push(&c.then);
					}
					stack.push(when.elze);
					break;
				}
				case Expression::Kind::Nil:
					assert(false);
			}
		} while (!stack.empty());
	}

	void emit_funs(Ctx& ctx, const FunsDeclarationOrder& funs) {
		std::unordered_set<ref<const Fun>> seen;
		for (const Fun& f : funs) {
			switch (f.body.kind()) {
				case AnyBody::Kind::CppSource:
					break;
				case AnyBody::Kind::Expression:
					each_dependent_fun(f.body.expression(), [&](ref<const Fun> referenced) {
						if (!seen.count(referenced)) {
							write_fun_header(ctx, f);
							ctx.out << ";\n";
							seen.insert({ referenced });
						}
					});
					break;
				case AnyBody::Kind::Nil:
					assert(false);
			}
			write_fun(ctx, f);
			seen.insert(ref(&f));
		}
	}

	void write_type_for_overload_name(Arena::StringBuilder& sb, const Type& type) {
		switch (type.kind()) {
			case Type::Kind::Param:
				sb << mangle{type.param()->name};
				break;
			case Type::Kind::Plain: {
				const PlainType& p = type.plain();
				sb << mangle{p.inst_struct.strukt->name};
				if (!p.inst_struct.type_arguments.empty()) throw "todo";
				break;
			}
		}
	}

	ArenaString escape_name(const Identifier& name, Arena& arena) {
		Arena::StringBuilder sb = arena.string_builder(100);
		sb << mangle{name};
		return sb.finish();
	}

	ArenaString escape_struct_name(const Identifier& module_name, const Identifier& struct_name, Arena& arena) {
		Arena::StringBuilder sb = arena.string_builder(100);
		sb << mangle{module_name} << "_" << mangle{struct_name};
		return sb.finish();
	}

	ArenaString escape_fun_name(const Fun& f, Arena& arena) {
		Arena::StringBuilder sb = arena.string_builder(100);
		sb << mangle{f.containing_module->name} << '_' << mangle{f.name} << '_';
		write_type_for_overload_name(sb, f.return_type);
		for (const Parameter& p : f.parameters) {
			sb << '_';
			write_type_for_overload_name(sb, p.type);
		}
		return sb.finish();
	}

	OverloadNames get_overload_names(const std::vector<ref<Module>>& modules) {
		Set<Identifier> module_names;

		// Map from a name to all structs with that name.
		MultiMap<Identifier, ref<const StructDeclaration>> global_structs_table;
		// Map from a name to all funs with that name.
		MultiMap<Identifier, ref<const Fun>> global_funs_table;

		for (ref<const Module> module : modules) {
			for (ref<const StructDeclaration> s : module->structs_declaration_order)
				global_structs_table.add(s->name, s);
			for (ref<const Fun> f : module->funs_declaration_order)
				global_funs_table.add(f->name, f);
			module_names.must_insert(module->name); //TODO: if 2 modules have the same name, harder to generate overload names
		}

		OverloadNames names;
		for (const auto& a : global_structs_table) {
			const Identifier& name = a.first;
			const std::vector<ref<const StructDeclaration>>& structs = a.second;
			for (ref<const StructDeclaration> s : structs)
				names.struct_names.must_insert(s, structs.size() == 1 ? escape_name(name, names.arena) : escape_struct_name(s->containing_module->name, name, names.arena));
		}

		for (const auto& a : global_funs_table) {
			const Identifier& name = a.first;
			const std::vector<ref<const Fun>>& funs = a.second;
			for (ref<const Fun> f : funs) {
				names.fun_names.must_insert(f, funs.size() == 1 ? escape_name(name, names.arena) : escape_fun_name(f, names.arena));
			}
		}

		return names;
	}
}

std::string emit(const std::vector<ref<Module>>& modules) {
	Writer out;
	out << "#include <iostream>\n#include <limits>\n";
	Ctx ctx { out, get_overload_names(modules) };
	for (const Module& module : modules) {
		emit_structs(ctx, module.structs_declaration_order);
		emit_funs(ctx, module.funs_declaration_order);
	}
	return out.finish();
}
