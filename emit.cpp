#include <unordered_map>
#include <unordered_set>

#include "emit.h"
#include "util/Map.h"

namespace {
	Option<StringSlice> mangle_char(char c) {
		switch (c) {
			case '+':
				return { "$add" };
			case '-':
				return { "$sub" };
			case '*':
				return { "$times" };
			case '/':
				return { "$div" };
			case '<':
				return { "$lt" };
			case '>':
				return { "$gt" };
			case '=':
				return { "$eq" };
			default:
				assert(('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'));
				return {};
		}
	}

	struct indent_t {} indent;
	struct dedent_t {} dedent;
	struct nl_t {} nl;

	class Writer {
		std::string out;
		uint _indent = 0;

	public:
		Writer() = default;
		Writer(const Writer& other) = delete;
		void operator=(const Writer& other) = delete;

		std::string finish() {
			return std::move(out);
		}

		Writer& operator<<(char s) {
			out += s;
			return *this;
		}
		Writer& operator<<(const char* s) {
			out += s;
			return *this;
		}
		Writer& operator<<(const StringSlice& s) {
			for (char c : s)
				out += c;
			return *this;
		}

		Writer& operator<<(nl_t) {
			out += '\n';
			for (uint i = 0; i != _indent; ++i)
				out += '\t';
			return *this;
		}
		Writer& operator<<(indent_t) { ++_indent; return *this; }
		Writer& operator<<(dedent_t) { --_indent; return *this; }
	};

	class OverloadNames {
		Arena arena;
		Map<ref<const Fun>, ArenaString> names;
		friend OverloadNames get_overload_names(const FunsTable& funs);
	public:
		StringSlice get_name(ref<const Fun> f) const {
			return names.get(f);
		}
	};

	struct Ctx {
		Writer& out;
		const OverloadNames& overload_names;
	};

	Writer& operator<<(Writer& out, const Type& t);

	void write_type_parameters(Writer& out, const DynArray<TypeParameter>& type_parameters) {
		if (type_parameters.empty()) return;
		out << "template <";
		for (uint i = 0; i != type_parameters.size(); ++i) {
			if (i != 0) out << ", ";
			out << "typename " << type_parameters[i].name;
		}
		out << '>' << nl;
	}

	struct indented { StringSlice s; };
	Writer& operator<<(Writer& out, indented i)  {
		for (char c : i.s) {
			out << c;
			if (c == '\n')
				out << '\t';
		}
		return out;
	}

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
		return out << i.strukt->name << i.type_arguments;
	}

	Writer& operator<<(Writer& out, const Type& t) {
		return t.is_parameter() ? out << t.param()->name : out << t.plain().inst_struct;
	}

	Writer& operator<<(Writer& out, const StructDeclaration& s) {
		write_type_parameters(out, s.type_parameters);
		switch (s.body.kind()) {
			case StructBody::Kind::Fields:
				out << "struct " << s.name << " {\n";
				for (const StructField& field : s.body.fields()) {
					const char TAB = '\t'; // https://youtrack.jetbrains.com/issue/CPP-12650
					out << TAB << field.type << ' ' << field.name << ";\n";
				}
				return out << "};\n\n";
			case StructBody::Kind::CppName:
				return out << "using " << s.name << " = " << s.body.cpp_name() << ";\n\n";
			case StructBody::Kind::CppBody:
				return out << "struct " << s.name << " {\n\t" << indented{s.body.cpp_body()} << "\n};\n\n";
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
				out << e.parameter()->name;
				break;
			case Expression::Kind::LocalReference:
				out << e.local_reference()->name;
				break;
			case Expression::Kind::StructFieldAccess: {
				const StructFieldAccess& sa = e.struct_field_access();
				bool p = needs_parens_before_dot(*sa.target);
				if (p) out << '(';
				ctx << *sa.target;
				if (p) out << ')';
				out << '.' << sa.field->name;
				break;
			}
			case Expression::Kind::Let:
				throw "todo";
			case Expression::Kind::Call: {
				const Call& c = e.call();
				out << c.called.fun->name << c.called.type_arguments << "(";
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
	Ctx& operator<<(Ctx& ctx, statement s) {
		const Expression& e = s.e;
		switch (e.kind()) {
			case Expression::Kind::Let: {
				const Let &l = e.let();
				ctx.out << l.type << ' ' << l.name << " = ";
				ctx << l.init;
				ctx.out << ';' << nl;
				ctx << statement{l.then};
				break;
			}
			case Expression::Kind::When: {
				const When &w = e.when();
				for (uint i = 0; i != w.cases.size(); ++i) {
					const Case &c = w.cases[i];
					if (i != 0) ctx.out << "} else ";
					ctx.out << "if (";
					ctx << c.cond;
					ctx.out << ") {" << indent << nl;
					ctx << statement{c.then};
					ctx.out << dedent << nl;
				}
				ctx.out << "} else {" << indent << nl;
				ctx << statement{w.elze};
				ctx.out << dedent << nl << '}';
				break;
			}
			case Expression::Kind::ParameterReference:
			case Expression::Kind::LocalReference:
			case Expression::Kind::StructFieldAccess:
			case Expression::Kind::Call:
			case Expression::Kind::StructCreate:
			case Expression::Kind::StringLiteral:
			case Expression::Kind::Nil:
				ctx.out << "return ";
				ctx << e;
				ctx.out << ';';
		}
		return ctx;
	}

	void write_body(Ctx ctx, const AnyBody& body) {
		switch (body.kind()) {
			case AnyBody::Kind::Expression:
				ctx.out << indent;
				ctx << statement{body.expression()};
				ctx.out << dedent;
				break;
			case AnyBody::Kind::CppSource:
				ctx.out << indented{body.cpp_source()};
				break;
			case AnyBody::Kind::Nil:
				assert(false);
		}
	}

	void emit_just_fun_header(Writer& out, const Fun& f, const OverloadNames& o) {
		out << f.return_type << " " << o.get_name(&f) << '(';
		bool first_param = true;
		for (const auto& param : f.parameters) {
			if (first_param)
				first_param = false;
			else
				out << ", ";
			out << "const " << param.type << "& " << param.name;
		}
		out << ')';
	}

	void emit_fun_header(Writer& out, const Fun& f, const OverloadNames& o) {
		emit_just_fun_header(out, f, o);
		out << ";\n";
	}

	void write_fun(Writer& out, const Fun& f, const OverloadNames& overload_names) {
		emit_just_fun_header(out, f, overload_names);
		out << " {\n\t";
		write_body({ out, overload_names }, f.body);
		out << "\n}\n\n";
	}

	enum class EmitState { Nil, Emitted, Emitting };

	template <typename Cb>
	void each_struct_field(const StructDeclaration& s, Cb cb) {
		if (s.body.kind() == StructBody::Kind::Fields) {
			for (const StructField& f : s.body.fields()) {
				if (f.type.is_plain())
					cb(f.type.plain().inst_struct.strukt);
			}
		}
	}

	void emit_structs(Writer& out, const Structs& structs) {
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
						out << *s;
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

	void emit_funs(Writer& out, const Funs& funs, const OverloadNames& overload_names) {
		std::unordered_set<ref<const Fun>> seen;
		for (const Fun& f : funs) {
			switch (f.body.kind()) {
				case AnyBody::Kind::CppSource:
					break;
				case AnyBody::Kind::Expression:
					each_dependent_fun(f.body.expression(), [&](ref<const Fun> referenced) {
						if (!seen.count(referenced)) {
							emit_fun_header(out, *referenced, overload_names);
							seen.insert({ referenced });
						}
					});
					break;
				case AnyBody::Kind::Nil:
					assert(false);
			}
			write_fun(out, f, overload_names);
			seen.insert(ref(&f));
		}
	}

	Arena::StringBuilder& operator<<(Arena::StringBuilder& sb, const Identifier& name) {
		for (char c : name.slice()) {
			auto m = mangle_char(c);
			if (m)
				sb << m.get();
			else
				sb << c;
		}
		return sb;
	}

	Arena::StringBuilder& operator<<(Arena::StringBuilder& sb, const Type& type) {
		switch (type.kind()) {
			case Type::Kind::Param:
				return sb << type.param()->name;
			case Type::Kind::Plain: {
				const PlainType& p = type.plain();
				sb << effect_name(p.effect) << '_' << p.inst_struct.strukt->name;
				if (!p.inst_struct.type_arguments.empty()) throw "todo";
				return sb;
			}
		}
	}

	ArenaString escape_name(const Identifier& name, Arena& arena) {
		return (arena.string_builder(100) << name).finish();
	}

	ArenaString make_overload_name(const Fun& f, Arena& arena) {
		Arena::StringBuilder sb = arena.string_builder(100);
		sb << f.name << '_' << f.return_type;
		for (const Parameter& p : f.parameters)
			sb << '_' << p.type;
		return sb.finish();
	}

	OverloadNames get_overload_names(const FunsTable& funs) {
		OverloadNames names;
		for (const std::pair<StringSlice, OverloadGroup>& overload_group : funs) {
			for (ref<const Fun> f : overload_group.second.funs)
				names.names.must_insert(f, overload_group.second.funs.size() == 1 ? escape_name(f->name, names.arena) : make_overload_name(*f, names.arena));
		}
		return names;
	}
}


std::string emit(const Module& module) {
	Writer out;
	emit_structs(out, module.structs);
	emit_funs(out, module.funs, get_overload_names(module.funs_table));
	return out.finish();
}
