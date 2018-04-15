#include "emit_body.h"

#include "./emit_type.h"
#include "./mangle.h"

namespace {
	struct BodyCtx {
		Writer& out;
		ref<const ConcreteFun> current_concrete_fun;
		Arena& scratch_arena;
		const Names& names;
		const ResolvedCalls& resolved_calls;
	};

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

	BodyCtx& operator<<(BodyCtx& ctx, const Expression& e) {
		Writer& out = ctx.out;
		switch (e.kind()) {
			case Expression::Kind::ParameterReference:
				out << mangle { e.parameter()->name };
				break;
			case Expression::Kind::LocalReference:
				out << mangle { e.local_reference()->name };
				break;
			case Expression::Kind::StructFieldAccess: {
				const StructFieldAccess& sa = e.struct_field_access();
				bool p = needs_parens_before_dot(*sa.target);
				if (p) out << '(';
				ctx << *sa.target;
				if (p) out << ')';
				out << '.' << ctx.names.get_name(sa.field);
				break;
			}
			case Expression::Kind::Let:
				throw "todo";
			case Expression::Kind::Seq:
				throw "todo";
			case Expression::Kind::Call: {
				const Call& c = e.call();
				// The function we're calling depends on the current ConcreteFun being emitted.
				ref<const ConcreteFun> called_concrete = ctx.resolved_calls.must_get({ ctx.current_concrete_fun, &c.called });
				out << ctx.names.get_name(called_concrete) << "(";
				for (uint i = 0; i != c.arguments.size(); ++i) {
					ctx << c.arguments[i];
					ctx.out << (i == c.arguments.size() - 1 ? ")" : ", ");
				}
				break;
			}
			case Expression::Kind::StructCreate: {
				// StructName<TypeArgs> { arg1, arg2 }
				const StructCreate& sc = e.struct_create();
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

	struct statement {
		const Expression& e;
	};
	struct return_statement {
		const Expression& e;
	};

	BodyCtx& write_statement(BodyCtx& ctx, const Expression& e, bool should_return);

	BodyCtx& operator<<(BodyCtx& ctx, statement s) { return write_statement(ctx, s.e, false); }

	BodyCtx& operator<<(BodyCtx& ctx, return_statement s) { return write_statement(ctx, s.e, true); }

	BodyCtx& write_statement(BodyCtx& ctx, const Expression& e, bool should_return) {
		switch (e.kind()) {
			case Expression::Kind::Let: {
				const Let& l = e.let();
				ctx.out << l.type << ' ' << mangle { l.name } << " = ";
				ctx << l.init;
				ctx.out << ';' << Writer::nl;
				ctx << return_statement { l.then };
				break;
			}
			case Expression::Kind::Seq: {
				const Seq& seq = e.seq();
				ctx << statement { seq.first };
				ctx.out << Writer::nl;
				ctx << return_statement { seq.then };
				break;
			}
			case Expression::Kind::When: {
				const When& w = e.when();
				for (uint i = 0; i != w.cases.size(); ++i) {
					const Case& c = w.cases[i];
					if (i != 0) ctx.out << "} else ";
					ctx.out << "if (";
					ctx << c.cond;
					ctx.out << ") {" << Writer::indent << Writer::nl;
					ctx << return_statement { c.then };
					ctx.out << Writer::dedent << Writer::nl;
				}
				ctx.out << "} else {" << Writer::indent << Writer::nl;
				ctx << return_statement { w.elze };
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
}

void emit_body(Writer& out, ref<const ConcreteFun> f, const Names& names, const ResolvedCalls& resolved_calls, Arena& scratch_arena) {
	const AnyBody& body = f->fun_declaration->body;
	switch (body.kind()) {
		case AnyBody::Kind::Expression: {
			out << Writer::indent;
			BodyCtx ctx { out, f, scratch_arena, names, resolved_calls };
			ctx << return_statement { body.expression() };
			out << Writer::dedent;
			break;
		}
		case AnyBody::Kind::CppSource:
			out << indented{body.cpp_source()};
			break;
		case AnyBody::Kind::Nil:
			assert(false);
	}
}
