#include <unordered_map>
#include <unordered_set>

#include "emit.h"

namespace {
	const char* mangle_char(char c) {
		switch (c) {
			case '+':
				return "$add";
			case '-':
				return "$sub";
			case '*':
				return "$times";
			case '/':
				return "$div";
			case '<':
				return "$lt";
			case '>':
				return "$gt";
			case '=':
				return "$eq";
			default:
				assert(('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'));
				return nullptr;
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
		Writer& operator<<(uint64_t u) {
			out += std::to_string(u);
			return *this;
		}
		Writer& operator<<(const ArenaString& a) {
			for (char c : a.slice())
				out += c;
			return *this;
		}
		Writer& operator<<(const Identifier& i) {
			for (char c : i.slice()) {
				auto m = mangle_char(c);
				if (m == nullptr)
					*this << c;
				else
					*this << m;
			}
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
		if (t.is_parameter())
			out << t.param()->name;
		else {
			const PlainType& p = t.plain();
			out << "/*" << effect_name(p.effect) << "*/ " << p.inst_struct;
		}
		return out;
	}

	Writer& operator<<(Writer& out, const StructDeclaration& s) {
		write_type_parameters(out, s.type_parameters);
		if (s.body.is_fields()) {
			out << "struct " << s.name << " {\n";
			for (const StructField& field : s.body.fields())
				out << '\t' << field.type << ' ' << field.name << ";\n";
			return out << "};\n\n";
		} else {
			return out << "using " << s.name << " = " << s.body.cpp_name() << ";\n\n";
		}
	}

	bool needs_parens_before_dot(const Expression& e) {
		switch (e.kind()) {
			case Expression::Kind::LocalReference:
			case Expression::Kind::ParameterReference:
			case Expression::Kind::StructFieldAccess:
			case Expression::Kind::Call:
			case Expression::Kind::StructCreate:
				return false;
			case Expression::Kind::Let:
			case Expression::Kind::UintLiteral:
			case Expression::Kind::When:
				return true;
			case Expression::Kind::Nil:
				assert(false);
		}
	}

	Writer& operator<<(Writer& out, const Expression& e) {
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
				out << *sa.target;
				if (p) out << ')';
				out << '.' << sa.field->name;
				break;
			}
			case Expression::Kind::Let:
				throw "todo";
			case Expression::Kind::Call: {
				const Call& c = e.call();
				out << c.called.fun->name << c.called.type_arguments << "(";
				for (uint i = 0;  i != c.arguments.size(); ++i)
					out << c.arguments[i] << (i == c.arguments.size() - 1 ? ")" : ", ");
				break;
			}
			case Expression::Kind::StructCreate: {
				// StructName { arg1, arg2 }
				const StructCreate& sc = e.struct_create();
				out << sc.inst_struct;
				out << " { ";
				for (uint i = 0;  i != sc.arguments.size(); ++i)
					out << sc.arguments[i] << (i == sc.arguments.size() - 1 ? " }" : ", ");
				break;
			}
			case Expression::Kind::UintLiteral:
				out << e.uint();
				break;
			case Expression::Kind::When:
				throw "todo";
			case Expression::Kind::Nil:
				assert(false);
		}
		return out;
	}

	struct statement { const Expression& e; };
	Writer& operator<<(Writer& out, statement s) {
		const Expression& e = s.e;
		switch (e.kind()) {
			case Expression::Kind::Let: {
				const Let &l = e.let();
				out << l.type << ' ' << l.name << " = " << l.init << ';' << nl << statement{l.then};
				break;
			}
			case Expression::Kind::When: {
				const When &w = e.when();
				for (uint i = 0; i != w.cases.size(); ++i) {
					const Case &c = w.cases[i];
					if (i != 0) out << "} else ";
					out << "if (" << c.cond << ") {" << indent << nl << statement{c.then} << dedent << nl;
				}
				out << "} else {" << indent << nl << statement{w.elze} << dedent << nl << '}';
				break;
			}
			case Expression::Kind::ParameterReference:
			case Expression::Kind::LocalReference:
			case Expression::Kind::StructFieldAccess:
			case Expression::Kind::Call:
			case Expression::Kind::StructCreate:
			case Expression::Kind::UintLiteral:
			case Expression::Kind::Nil:
				out << "return " << e << ';';
		}
		return out;
	}

	Writer& operator<<(Writer& out, const AnyBody& body) {
		switch (body.kind()) {
			case AnyBody::Kind::Expression:
				return out << indent << statement{body.expression()} << dedent;
			case AnyBody::Kind::CppSource:
				return out << body.cpp_source(); //TODO: output indented string
			case AnyBody::Kind::Nil:
				assert(false);
		}
	}

	void emit_just_fun_header(Writer& out, const Fun& f) {
		out << f.return_type << " " << f.name << '(';
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

	void emit_fun_header(Writer& out, const Fun& f) {
		emit_just_fun_header(out, f);
		out << ";\n";
	}

	Writer& operator<<(Writer& out, const Fun& f) {
		emit_just_fun_header(out, f);
		return out << " {\n\t" << f.body << "\n}\n\n";
	}

	enum class EmitState { Nil, Emitted, Emitting };

	template <typename Cb>
	void each_struct_field(const StructDeclaration& s, Cb cb) {
		if (s.body.is_fields()) {
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
				case Expression::Kind::UintLiteral:
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

	void emit_funs(Writer& out, const Funs& funs) {
		std::unordered_set<ref<const Fun>> seen;
		for (const Fun& f : funs) {
			switch (f.body.kind()) {
				case AnyBody::Kind::CppSource:
					break;
				case AnyBody::Kind::Expression:
					each_dependent_fun(f.body.expression(), [&out, &seen](ref<const Fun> referenced) {
						if (!seen.count(referenced)) {
							emit_fun_header(out, *referenced);
							seen.insert({ referenced });
						}
					});
					break;
				case AnyBody::Kind::Nil:
					assert(false);
			}
			out << f;
			seen.insert(ref(&f));
		}
	}
}

std::string emit(const Module& module) {
	Writer out;
	emit_structs(out, module.structs);
	emit_funs(out, module.funs);
	return out.finish();
}
