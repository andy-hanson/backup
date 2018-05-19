#include "emit_body.h"

#include "../compile/model/type_of_expr.h"
#include "./substitute_type_arguments.h"

namespace {
	using Statements = MaxSizeVector<16, CStatement>;

	struct BodyCtx {
		Arena& out_arena;
		Ref<const ConcreteFun> current_concrete_fun;
		ConcreteFunsCache& concrete_funs;
		EmittableTypeCache& type_cache;
		MaxSizeVector<16, Ref<const ConcreteFun>>& to_emit;
		const BuiltinTypes& builtin_types;
		uint next_temp;
	};

	class OutVar {
	public:
		// Void: This is a void statement, so do nothing.
		// Pointer: Value is already a pointer. Write with `*v = ...`
		// Local: Value is a local variable. Write with `v = ...`
		enum Kind { Void, WriteToPointer, WriteToLocal };
	private:
		Kind _kind;
		Option<CAssignLhs> _write_to;

		OutVar(Kind kind, Option<CAssignLhs> write_to) : _kind{kind}, _write_to{write_to} {}
	public:
		inline static OutVar return_out_parameter() {
			return { Kind::WriteToPointer, Option { CAssignLhs::ret() } };
		}
		inline static OutVar for_field(const OutVar& parent, const StructField& field, Arena& out_arena) {
			assert(parent._kind != Kind::Void);
			return OutVar {
				Kind::WriteToPointer,
				Option { CAssignLhs { CPropertyAccess { out_arena.put(parent.lhs().to_expression()), parent._kind == Kind::WriteToPointer, &field } } },
			};
		}
		inline OutVar(uint tmp) : _kind{Kind::WriteToLocal}, _write_to{CAssignLhs{CVariableName{tmp}}} {}
		inline OutVar(Identifier name) : _kind{Kind::WriteToLocal}, _write_to{CAssignLhs{CVariableName{name}}} {}

		inline Kind kind() const { return _kind; }
		inline bool is_write_to() const { return _kind == Kind::WriteToLocal || _kind == Kind::WriteToPointer; }

		/** Creates the LHS of an assignment to write to, as in `*x = 0;` */
		inline CAssignLhs lhs() const {
			assert(_kind != Kind::Void);
			return _write_to.get();
		}
		/** Writes this as an out argument, as in `zero(x)`. */
		inline CExpression out_arg(Arena& arena) const {
			return CExpression { CAddressOfExpression { arena.put(_write_to.get().to_expression()) } };
		}
	};

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

	CExpression emit_arg(BodyCtx& ctx, Statements& statements, const Expression& e, bool is_pointer);

	//TODO:MOVE down
	CExpression emit_as_simple_expression(const BodyCtx& ctx, const Expression& e, bool is_pointer) {
		assert(!needs_temporary_local(e));
		switch (e.kind()) {
			case Expression::Kind::ParameterReference: {
				// Parameters should already be pointers.
				CExpression ce { CVariableName { e.parameter_reference()->name }};
				return is_pointer ? ce : CExpression { CDereferenceExpression { ctx.out_arena.put(ce) }};
			}
			case Expression::Kind::LocalReference: {
				Ref<const Let> l = e.local_reference();
				CExpression ce { CVariableName { l->name } };
				return l->type.lifetime().is_pointer() ? CExpression { CAddressOfExpression { ctx.out_arena.put(ce) } } : ce;
			}
			case Expression::Kind::StructFieldAccess: {
				// &p->x or &p.x
				const StructFieldAccess& sa = e.struct_field_access();
				CExpression target = emit_as_simple_expression(ctx, sa.target, false);
				CPropertyAccess access { ctx.out_arena.put(target), type_of_expr(sa.target, ctx.builtin_types).lifetime().is_pointer(), sa.field };
				return CExpression { CDereferenceExpression { ctx.out_arena.put(CExpression { access }) } };
			}
			case Expression::Kind::StringLiteral:
				return CExpression { e.string_literal() };
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

	void emit_expression_as_statement(BodyCtx& ctx, Statements& statements, const OutVar& out_var, const Expression& e);

	CStatement to_block_or_statement(const Statements& statements, Arena& out_arena) {
		return statements.size() == 1
			   ? statements[0]
			   : CStatement { CBlockStatement { to_arena(statements, out_arena) } };
	}
	CStatement emit_expression_to_statement_or_block(BodyCtx& ctx, const OutVar& out_var, const Expression& e) {
		Statements statements;
		emit_expression_as_statement(ctx, statements, out_var, e);
		return to_block_or_statement(statements, ctx.out_arena);
	}

	EmittableType get_emittable_type(const BodyCtx& ctx, const Type& type) {
		return ctx.type_cache.get_type(type, ctx.current_concrete_fun->fun_declaration->signature.type_parameters, ctx.current_concrete_fun->type_arguments);
	}

	void emit_void(BodyCtx& ctx, Statements& statements, const Expression& e) {
		// Just create a dummy Void value to write to.
		uint temp_id = ctx.next_temp;
		++ctx.next_temp;
		EmittableType void_type = get_emittable_type(ctx, ctx.builtin_types.void_type.get());
		statements.push(CStatement { CLocalDeclaration { void_type, CVariableName { temp_id }, {} } });
		emit_expression_as_statement(ctx, statements, OutVar { temp_id }, e);
	}

	void emit_let(BodyCtx& ctx, Statements& statements, const OutVar& out_var, const Let& let) {
		EmittableType type = get_emittable_type(ctx, let.type);
		if (needs_temporary_local(let.init)) {
			statements.push(CStatement { CLocalDeclaration { type, CVariableName { let.name }, {} } });
			emit_expression_as_statement(ctx, statements, OutVar { let.name }, let.init);
		} else {
			CExpression init = emit_as_simple_expression(ctx, let.init, /*needs_pointer*/ let.type.lifetime().is_pointer());
			statements.push(CStatement { CLocalDeclaration { type, CVariableName { let.name }, Option { init } } });
		}
		emit_expression_as_statement(ctx, statements, out_var, let.then);
	}

	//TODO:MOVE
	template <typename T>
	struct all_but_first_reverse_iter {
		const Slice<T>& slice;

		struct const_iterator {
			const T* ptr;

			void operator++() {
				--ptr;
			}
			const T& operator*() {
				return *ptr;
			}
			friend bool operator!=(const const_iterator& a, const const_iterator& b) {
				return a.ptr != b.ptr;
			}
		};

		const_iterator begin() { return const_iterator { slice.end() - 1 }; }
		const_iterator end() { return const_iterator { slice.begin() }; } // Don't include begin()
	};

	void emit_when(BodyCtx& ctx, Statements& statements, const OutVar& out_var, const When& when) {
		Ref<const CStatement> elze = ctx.out_arena.put(emit_expression_to_statement_or_block(ctx, out_var, when.elze));

		for (const Case& c : all_but_first_reverse_iter<Case>{ when.cases }) {
			//If this is the first case, emit directly instead of to a buffer.
			Statements temp_statements;
			CExpression cond = emit_arg(ctx, temp_statements, c.cond, /*is_pointer*/ false);
			Ref<const CStatement> then = ctx.out_arena.put(emit_expression_to_statement_or_block(ctx, out_var, c.then));
			temp_statements.push(CStatement { CIfStatement { cond, then, ctx.out_arena.put(elze) } });
			elze = ctx.out_arena.put(to_block_or_statement(temp_statements, ctx.out_arena));
		}

		const Case& case0 = when.cases[0];
		CExpression cond0 = emit_arg(ctx, statements, case0.cond, /*is_pointer*/ false);
		Ref<const CStatement> then0 = ctx.out_arena.put(emit_expression_to_statement_or_block(ctx, out_var, case0.then));
		statements.push(CStatement { CIfStatement { cond0, then0, elze } });
	}

	void emit_call(BodyCtx& ctx, Statements& statements, const OutVar& out_var, const Call& call) {
		TryInsertResult<ConcreteFun> got_fun = ctx.concrete_funs.get_concrete_fun_for_call(ctx.current_concrete_fun, call.called, ctx.type_cache);
		if (got_fun.was_inserted)
			ctx.to_emit.push(got_fun.value);

		Slice<CExpression> arguments = map_with_first<CExpression>{}(
			ctx.out_arena,
			out_var.is_write_to() ? Option { out_var.out_arg(ctx.out_arena) } : Option<CExpression>{},
			call.arguments, [&](const Expression& arg) {
				return emit_arg(ctx, statements, arg, /*is_pointer*/ true);
			});
		statements.push(CStatement { CCall { got_fun.value, arguments } });
	}

	void emit_expression_as_statement(BodyCtx& ctx, Statements& statements, const OutVar& out_var, const Expression& e) {
		switch (e.kind()) {
			case Expression::Kind::Let:
				emit_let(ctx, statements, out_var, e.let());
				break;

			case Expression::Kind::Seq: {
				const Seq& seq = e.seq();
				emit_void(ctx, statements, seq.first);
				emit_expression_as_statement(ctx, statements, out_var, seq.then);
				break;
			}

			case Expression::Kind::When:
				emit_when(ctx, statements, out_var, e.when());
				break;

			case Expression::Kind::Assert: {
				const Expression& asserted = e.asserted();
				CExpression arg = emit_arg(ctx, statements, asserted, /*is_pointer*/ false);
				statements.push(CStatement { CAssert { arg } });
				[[fallthrough]];
			}
			case Expression::Kind::Pass:
				// Since Void is an empty type, don't bother writing to it.
				break;

			case Expression::Kind::Call:
				emit_call(ctx, statements, out_var, e.call());
				break;

			case Expression::Kind::StructCreate: {
				const StructCreate& s = e.struct_create();
				assert(out_var.is_write_to()); // Should never 'return' a struct
				// Write to each field individually.
				zip(s.inst_struct.strukt->body.fields(), s.arguments, [&](const StructField& field, const Expression& arg) {
					emit_expression_as_statement(ctx, statements, OutVar::for_field(out_var, field, ctx.out_arena), arg);
				});
				todo(); //Need to write the result out
				//break;
			}

			case Expression::Kind::ParameterReference:
			case Expression::Kind::LocalReference:
			case Expression::Kind::StructFieldAccess:
			case Expression::Kind::StringLiteral:
				statements.push(CStatement { CAssignStatement { out_var.lhs(), emit_as_simple_expression(ctx, e, /*needs_pointer*/ out_var.kind() == OutVar::Kind::WriteToPointer) } });
				break;

			case Expression::Kind::Nil:
			case Expression::Kind::Bogus:
				unreachable();
		}
	}

	CExpression emit_arg(BodyCtx& ctx, Statements& statements, const Expression& e, bool is_pointer) {
		if (needs_temporary_local(e)) {
			uint temp_id = ctx.next_temp;
			++ctx.next_temp;
			statements.push(CStatement { CLocalDeclaration { get_emittable_type(ctx, type_of_expr(e, ctx.builtin_types)), CVariableName { temp_id }, /*initializer*/ {} } });
			emit_expression_as_statement(ctx, statements, OutVar { temp_id }, e);
			CExpression var { CVariableName { temp_id } };
			return is_pointer ? CExpression { CAddressOfExpression { ctx.out_arena.put(var) } } : var;
		} else
			return emit_as_simple_expression(ctx, e, is_pointer);
	}
}

CFunctionBody emit_body(
	Ref<const ConcreteFun> f,
	const BuiltinTypes& builtin_types,
	ConcreteFunsCache& concrete_funs,
	EmittableTypeCache& types_cache,
	ToEmit& to_emit,
	Arena& out_arena) {
	const AnyBody& body = f->fun_declaration->body;
	switch (body.kind()) {
		case AnyBody::Kind::Expr: {
			BodyCtx ctx { out_arena, f, concrete_funs, types_cache, to_emit, builtin_types, /*next_tmp*/ 0 };
			Statements statements;
			emit_expression_as_statement(ctx, statements, OutVar::return_out_parameter(), body.expression());
			return CFunctionBody { to_arena(statements, out_arena) };
		}
		case AnyBody::Kind::CppSource:
			return CFunctionBody { body.cpp_source().slice() };
		case AnyBody::Kind::Nil:
			unreachable();
	}
}
