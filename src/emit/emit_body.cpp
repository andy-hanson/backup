#include "emit_body.h"

#include "../compile/model/type_of_expr.h"
#include "./emit_type.h"
#include "./mangle.h"
#include "./substitute_type_arguments.h"

namespace {
	struct BodyCtx {
		Writer& out;
		Ref<const ConcreteFun> current_concrete_fun;
		const Names& names;
		const ResolvedCalls& resolved_calls;
		const BuiltinTypes& builtin_types;
		uint next_tmp;
	};

	const StringSlice TMP = "_tmp_";
	const StringSlice RET = "_ret";

	class IdOrTmp {
		enum class Kind { Name, Tmp, Ret };
		union Data {
			Identifier name;
			uint tmp;
			Data() {}
		};

		Kind _kind;
		Data _data;

		IdOrTmp(Kind kind) : _kind(kind) {}

	public:
		inline static IdOrTmp ret() { return { Kind::Ret }; }
		inline explicit IdOrTmp(uint tmp) : _kind(Kind::Tmp) { _data.tmp = tmp; }
		inline explicit IdOrTmp(Identifier name) : _kind(Kind::Name) { _data.name = name; }

		friend Writer& operator<<(Writer& out, const IdOrTmp& id) {
			switch (id._kind) {
				case Kind::Name:
					return out << mangle{id._data.name};
				case Kind::Tmp:
					return out << TMP << id._data.tmp;
				case Kind::Ret:
					return out << RET;
			}
		}
	};

	class WriteTo;
	struct WriteToAndProperty {
		Ref<const WriteTo> target;
		StringSlice property_name;
	};

	class WriteTo {
		enum class Kind { Id, Property };
		union Data {
			IdOrTmp identifier;
			WriteToAndProperty and_property;
			Data() {}
		};
		Kind _kind;
		Data _data;
	public:
		inline WriteTo(IdOrTmp id) : _kind{Kind::Id} { _data.identifier = id; }
		inline WriteTo(WriteToAndProperty p) : _kind{Kind::Property} { _data.and_property = p; }

		// Write as in `*out = foo;`.
		void write_as_lhs(Writer& out, bool first_is_pointer) const {
			switch (_kind) {
				case Kind::Id:
					if (first_is_pointer) out << '*';
					out << _data.identifier;
					break;
				case Kind::Property: {
					const WriteToAndProperty& p = _data.and_property;
					if (p.target->_kind == Kind::Id) {
						out << p.target->_data.identifier << (first_is_pointer ? "->" : ".") << p.property_name;
					} else {
						// property of a property
						todo();
					}
					break;
				}
			}
		}

		void write_as_out_parameter(Writer& out, bool first_is_pointer) const {
			switch (_kind) {
				case Kind::Id:
					if (!first_is_pointer) out << '&';
					out << _data.identifier;
					break;
				case Kind::Property:
					const WriteToAndProperty& p = _data.and_property;
					if (p.target->_kind == Kind::Id) {
						out << '&' << p.target->_data.identifier << (first_is_pointer ? "->" : ".") << p.property_name;
					} else {
						// property of a property
						p.target->write_as_out_parameter(out, first_is_pointer);
						out << '.' << p.property_name;
					}
			}
		}
	};

	class OutVar {
	public:
		// Void: This is a void statement, so do nothing.
		// Return: should 'return' instead of writing to a pointer.
		// Pointer: Value is already a pointer. Write with `*v = ...`
		// Local: Value is a local variable. Write with `v = ...`
		enum Kind { Void, Return, WriteToPointer, WriteToLocal };
	private:
		Kind _kind;
		Option<WriteTo> _write_to;

	public:
		OutVar(Kind kind, Option<WriteTo> write_to) : _kind{kind}, _write_to{write_to} {}
		static OutVar return_direct() { return { Kind::Return, {} }; }
		static OutVar return_out_parameter() { return { Kind::WriteToPointer, Option { WriteTo { IdOrTmp::ret() } } }; }
		OutVar(uint tmp) : _kind{Kind::WriteToLocal}, _write_to{IdOrTmp{tmp}} {}
		OutVar(Identifier name) : _kind{Kind::WriteToLocal}, _write_to{IdOrTmp{name}} {}

		inline Kind kind() const { return _kind; }
		inline bool is_return() const { return _kind == Kind::Return; }
		inline bool is_write_to() const { return _kind == Kind::WriteToLocal || _kind == Kind::WriteToPointer; }

		inline const WriteTo& write_to() const {
			assert(is_write_to());
			return _write_to.get();
		}
		void write_as_lhs(Writer& out) const {
			write_to().write_as_lhs(out, _kind == Kind::WriteToPointer);
		}
		void write_as_out_parameter(Writer& out) const {
			write_to().write_as_out_parameter(out, _kind == Kind::WriteToPointer);
		}
	};

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


	bool needs_temporary_local(const Expression& e) {
		switch (e.kind()) {
			case Expression::Kind::ParameterReference:
			case Expression::Kind::LocalReference:
			case Expression::Kind::StringLiteral:
				return false;
			case Expression::Kind::StructFieldAccess:
				//true if lhs needs temporary
				return needs_temporary_local(e.struct_field_access().target);
			case Expression::Kind::StructCreate:
			case Expression::Kind::Let:
			case Expression::Kind::Seq:
			case Expression::Kind::When:
			case Expression::Kind::Call: // TODO: call only needs a temprorary if one of its arguments does, or the return is 'new' (goes in a local).
				return true;
			case Expression::Kind::Assert:
			case Expression::Kind::Pass:
			case Expression::Kind::Nil:
			case Expression::Kind::Bogus:
				unreachable();
		}
	}

	Option<uint> emit_arg_begin(BodyCtx& ctx, const Expression& e);
	void emit_arg_end(BodyCtx& ctx, const Expression& e, Option<uint> tmp);

	//TODO:MOVE down
	void emit_as_simple_expression(BodyCtx& ctx, const Expression& e) {
		assert(!needs_temporary_local(e));
		switch (e.kind()) {
			case Expression::Kind::ParameterReference:
				ctx.out << mangle { e.parameter_reference()->name };
				break;
			case Expression::Kind::LocalReference:
				ctx.out << mangle { e.local_reference()->name };
				break;
			case Expression::Kind::StructFieldAccess: {
				const StructFieldAccess& sa = e.struct_field_access();
				ctx.out << '&';
				emit_as_simple_expression(ctx, sa.target);
				ctx.out << (type_of_expr(sa.target, ctx.builtin_types).lifetime().is_borrow() ? "->" : ".");
				ctx.out << ctx.names.get_name(sa.field);
				break;
			}
			case Expression::Kind::StringLiteral:
				write_string_literal(ctx.out, e.string_literal());
				break;
			case Expression::Kind::Let:
			case Expression::Kind::Seq:
			case Expression::Kind::When:
			case Expression::Kind::Assert:
			case Expression::Kind::Pass:
			case Expression::Kind::Nil:
			case Expression::Kind::Bogus:
			case Expression::Kind::Call:
			case Expression::Kind::StructCreate:
				unreachable();
		}
	}

	void emit_expression_as_statement(BodyCtx& ctx, const OutVar& out_var, const Expression& e);

	void emit_void(BodyCtx& ctx, const Expression& e) {
		// Just create a dummy Void value to write to.
		uint tmp_id = ctx.next_tmp;
		++ctx.next_tmp;
		ctx.out << "Void _tmp_" << tmp_id << ';' << Writer::nl;
		emit_expression_as_statement(ctx, OutVar { tmp_id }, e);
	}

	void emit_expression_as_statement(BodyCtx& ctx, const OutVar& out_var, const Expression& e) {
		switch (e.kind()) {
			case Expression::Kind::Let: {
				const Let& l = e.let();
				substitute_and_write_inst_struct(ctx.out, ctx.current_concrete_fun, l.type.stored_type(), ctx.names);
				ctx.out << ' ' << mangle{l.name} << ';' << Writer::nl;
				if (needs_temporary_local(l.init)) {
					emit_expression_as_statement(ctx, OutVar { l.name }, l.init);
				} else {
					ctx.out << " = ";
					emit_as_simple_expression(ctx, l.init);
					ctx.out << ';';
				}
				ctx.out << Writer::nl;
				emit_expression_as_statement(ctx, out_var, l.then);
				break;
			}
			case Expression::Kind::Seq: {
				const Seq& seq = e.seq();
				emit_void(ctx, seq.first);
				emit_expression_as_statement(ctx, out_var, seq.then);
				break;
			}

			case Expression::Kind::When: {
				const When& w = e.when();

				for (const Case& c : w.cases) {
					Option<uint> tmp = emit_arg_begin(ctx, c.cond);
					ctx.out << "if (";
					emit_arg_end(ctx, c.cond, tmp);
					ctx.out << ") {" << Writer::indent << Writer::nl;
					emit_expression_as_statement(ctx, out_var, c.then);
					ctx.out << Writer::dedent << Writer::nl << "} else {" << Writer::indent << Writer::nl;
				}

				emit_expression_as_statement(ctx, out_var, w.elze);
				for (uint i = w.cases.size(); i != 0; --i)
					ctx.out << '}' << Writer::dedent << Writer::nl;
				break;
			}

			case Expression::Kind::Assert: {
				const Expression& asserted = e.asserted();
				Option<uint> tmp = emit_arg_begin(ctx, asserted);
				ctx.out << "assert(";
				emit_arg_end(ctx, asserted, tmp);
				ctx.out << ");" << Writer::nl;
				[[fallthrough]];
			}
			case Expression::Kind::Pass:
				assert(out_var.kind() != OutVar::Kind::Return);
				// Since Void is an empty type, don't bother writing to it.
				break;

			case Expression::Kind::Call: {
				const Call& call = e.call();

				MaxSizeVector<8, Option<uint>> tmps = max_size_map<8, Option<uint>>()(call.arguments, [&](const Expression& arg) { return emit_arg_begin(ctx, arg); });

				if (out_var.is_return())
					ctx.out << "return ";

				Ref<const ConcreteFun> called = ctx.resolved_calls.must_get(ConcreteFunAndCalled { ctx.current_concrete_fun, &call.called });
				ctx.out << ctx.names.get_name(called) << '(';

				bool first_arg = true;
				if (out_var.is_write_to()) {
					out_var.write_as_out_parameter(ctx.out);
					first_arg = false;
				}

				zip(tmps, call.arguments, [&](const Option<uint>& tmp, const Expression& arg) {
					if (first_arg)
						first_arg = false;
					else
						ctx.out << ", ";
					emit_arg_end(ctx, arg, tmp);
				});

				ctx.out << ");";
				break;
			}

			case Expression::Kind::StructCreate: {
				const StructCreate& s = e.struct_create();
				assert(out_var.is_write_to()); // Should never 'return' a struct
				// Write to each field individually.
				zip_with_is_last(s.inst_struct.strukt->body.fields(), s.arguments, [&](const StructField& field, const Expression& arg, bool is_last) {
					OutVar field_out_var { out_var.kind(), Option { WriteTo { WriteToAndProperty { &out_var.write_to(), field.name } } } };
					emit_expression_as_statement(ctx, field_out_var, arg);
					if (!is_last) ctx.out << Writer::nl;
				});
				break;
			}

			case Expression::Kind::ParameterReference:
			case Expression::Kind::LocalReference:
			case Expression::Kind::StructFieldAccess:
			case Expression::Kind::StringLiteral:
				switch (out_var.kind()) {
					case OutVar::Kind::Void:
						unreachable();
					case OutVar::Kind::Return:
						ctx.out << "return ";
						break;
					case OutVar::Kind::WriteToPointer:
					case OutVar::Kind::WriteToLocal:
						out_var.write_as_lhs(ctx.out);
						// Since this is simple, use a direct assignment.
						ctx.out << " = ";
				}
				emit_as_simple_expression(ctx, e);
				ctx.out << ';';
				break;

			case Expression::Kind::Nil:
			case Expression::Kind::Bogus:
				unreachable();
		}
	}

	Option<uint> emit_arg_begin(BodyCtx& ctx, const Expression& e) {
		if (needs_temporary_local(e)) {
			substitute_and_write_inst_struct(ctx.out, ctx.current_concrete_fun, type_of_expr(e, ctx.builtin_types).stored_type(), ctx.names);
			uint tmp_id = ctx.next_tmp;
			++ctx.next_tmp;
			ctx.out << " _tmp_" << tmp_id << ';' << Writer::nl;
			emit_expression_as_statement(ctx, OutVar { tmp_id }, e);
			ctx.out << Writer::nl;
			return Option { tmp_id };
		} else return {};
	}
	void emit_arg_end(BodyCtx& ctx, const Expression& e, Option<uint> tmp) {
		if (needs_temporary_local(e)) {
			ctx.out << IdOrTmp { tmp.get() };
		} else {
			assert(!tmp.has());
			emit_as_simple_expression(ctx, e);
		}
	}
}

void emit_body(Writer& out, Ref<const ConcreteFun> f, const Names& names, const BuiltinTypes& builtin_types, const ResolvedCalls& resolved_calls) {
	const AnyBody& body = f->fun_declaration->body;
	switch (body.kind()) {
		case AnyBody::Kind::Expr: {
			out << Writer::indent;
			BodyCtx ctx { out, f, names, resolved_calls, builtin_types, /*next_tmp*/ 0 };
			OutVar out_var = f->fun_declaration->signature.return_type.lifetime().is_borrow()  ? OutVar::return_direct() : OutVar::return_out_parameter();
			emit_expression_as_statement(ctx, out_var, body.expression());
			out << Writer::dedent;
			break;
		}
		case AnyBody::Kind::CppSource:
			out << indented{body.cpp_source()};
			break;
		case AnyBody::Kind::Nil:
			unreachable();
	}
}
