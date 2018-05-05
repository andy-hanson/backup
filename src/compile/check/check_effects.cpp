#include "./check_effects.h"

#include "../model/expr.h"
#include "../../util/collection_util.h"
#include "../../util/MaxSizeVector.h"
#include "../../util/SmallMap.h"

namespace {
	// Sorted by parameter index.
	using Params = MaxSizeVector<4, Ref<const Parameter>>;
	// Unordered.
	using Locals = MaxSizeVector<4, Ref<const Let>>;

	// NOTE: this is more complicated than combine_locals because we need to preserve the order.
	void combine_parameters(Params& a, const Params& b) {
		uint ai = 0; // Use a numeric index because it may resize and invalidate iterators.
		Params::const_iterator bi = b.begin();

		// Walk along them together, inserting anything from 'b' not in 'a'.
		while (true) {
			if (ai == a.size()) {
				while (bi != b.end()) {
					a.push(*bi);
					++bi;
				}
				break;
			} else if (bi == b.end())
				break;

			const Parameter& pa = a[ai];
			Ref<const Parameter> pb = *bi;
			if (pa.index < pb->index) {
				// Something in a is not in b.
				++ai;
			} else if (pa.index > pb->index) {
				// Need to insert from b to a.
				a.insert(ai, pb);
				++bi;
			} else {
				assert(pa.index == pb->index);
				++ai;
				++bi;
			}
		}
	}

	void combine_locals(Locals& a, const Locals& b) {
		for (Ref<const Let> rb : b)
			if (!contains(a, rb))
				a.push(rb);
	}

	// An effect may be:
	// * Own
	// * Borrow, of any combination of parameters/own locals
	//   (we don't borrow borrowed locals, we just transitively borrow what they borrow)
	class ExprEffect {
		// If this is New, then _params and _locals should be empty.
		// Else, this is from a declared reduction on a return type.
		Option<Effect> _declared;
		Params _params;
		Locals _locals; // only used if kind is Local

		ExprEffect(Effect e) : _declared(e) { assert(e == Effect::EOwn); }
	public:
		static ExprEffect own() { return ExprEffect{Effect::EOwn}; }
		ExprEffect(Ref<const Parameter> p) : _params(p) {}
		// Construct from an *own* local.
		ExprEffect(Ref<const Let> local) : _locals(local) {}
		void operator=(const ExprEffect& other) {
			_declared = other._declared;
			_params = other._params;
			_locals = other._locals;
		}
		ExprEffect(const ExprEffect& other) : _declared(other._declared), _params(other._params), _locals(other._locals) {} //todo:perf

		bool is_own() const { return _declared.has() && _declared.get() == Effect::EOwn; }
		const Params& params() const { return _params; }
		const Locals& locals() const { return _locals; }

		bool is_sufficient(Effect expected) const {
			switch (expected) {
				case Effect::EGet:
					return true; // Everything is gettable.
				case Effect::ESet:
				case Effect::EIo:
					if (_declared.has() && _declared.get() < expected)
						return false;
					return every(_params, [&](Ref<const Parameter> p) { return p->effect < expected; });
					// Don't bother with _locals -- that is only for own locals, so we can do anything with those.
				case Effect::EOwn:
					return is_own();
			}
		}

		void combine(const ExprEffect& other) {
			//We combine 'from' arguments -- those should never be 'own'
			assert(!is_own());
			assert(!other.is_own());
			if (other._declared.has())
				lessen(other._declared.get());
			combine_parameters(_params, other._params);
			combine_locals(_locals, other._locals);
		}

		// A signature can declare a return effect that's less than the actual effect.
		void lessen(Effect e) {
			assert(e != Effect::EOwn);
			if (_declared.has()) {
				assert(_declared.get() != Effect::EOwn);
				_declared = effect::min(_declared.get(), e);
			} else {
				_declared = e;
			}
		}
	};

	struct Ctx {
		SmallMap<16, Ref<const Let>, ExprEffect> local_effects;
	};

	ExprEffect infer_effect(Expression& e, Ctx& ctx);

	ExprEffect infer_call_effect(Call& call, Ctx& ctx) {
		const CalledDeclaration& d = call.called.called_declaration;
		const FunSignature& sig = d.sig();

		Option<ExprEffect> return_effect;

		//Also check that we provide the effects it expects.
		zip(sig.parameters, call.arguments, [&](const Parameter& parameter, Expression& argument) {
			ExprEffect arg_effect = infer_effect(argument, ctx);
			// Note: allowed to pass an 'own' object where a reference is expected -- we will just delete it after.
			// But if the return *borrows* from an own object, that's a no-no.
			if (!arg_effect.is_sufficient(parameter.effect))
				throw "todo";

			if (parameter.from) {
				assert(parameter.effect != Effect::EOwn);
				if (arg_effect.is_own())
					throw "Todo"; // Can't borrow from 'own' arg.

				if (return_effect.has()) {
					return_effect.get().combine(arg_effect);
				} else {
					return_effect = arg_effect;
				}
			}
		});

		if (return_effect.has()) {
			assert(sig.effect != Effect::EOwn);
			return_effect.get().lessen(sig.effect);
			return return_effect.get();
		} else {
			assert(sig.effect == Effect::EOwn);
			return ExprEffect::own();
		}
	}

	ExprEffect infer_effect(Expression& e, Ctx& ctx) {
		switch (e.kind()) {
			case Expression::Kind::Nil: unreachable();
			case Expression::Kind::Bogus:
			case Expression::Kind::StringLiteral: // 'String' type is a slice, and the memory is in the executable so no need to worry about lifetime.
			case Expression::Kind::Pass:
				return ExprEffect::own();
			case Expression::Kind::ParameterReference:
				return ExprEffect { e.parameter() };
			case Expression::Kind::LocalReference: {
				Ref<const Let> l = e.local_reference();
				const ExprEffect& local_effect = ctx.local_effects.must_get(l);
				// If we own a local, can borrow it for any purpose.
				return local_effect.is_own() ? ExprEffect { l } : local_effect;
			}
			case Expression::Kind::StructFieldAccess:
				return infer_effect(e.struct_field_access().target, ctx);
			case Expression::Kind::Let: {
				Let& l = e.let();
				ExprEffect init_effect = infer_effect(l.init, ctx);
				l.is_own.init(init_effect.is_own());
				ctx.local_effects.push(&l, init_effect);
				ExprEffect res = infer_effect(l.then, ctx);
				ctx.local_effects.pop();
				return res;
			}
			case Expression::Kind::Seq: {
				Seq& seq = e.seq();
				ExprEffect first_effect = infer_effect(seq.first, ctx); // Void should always be Get
				assert(first_effect.is_own()); // It should return Void, which is Own.
				return infer_effect(seq.then, ctx);
			}
			case Expression::Kind::Call:
				return infer_call_effect(e.call(), ctx);
			case Expression::Kind::StructCreate:
				throw "todo"; // Need to check that struct members can be made own
				//return ExprEffect::own();
			case Expression::Kind::When:
				throw "todo";
			case Expression::Kind::Assert:
				ExprEffect eff = infer_effect(e.asserted(), ctx);
				assert(eff.is_own());
				return ExprEffect::own(); // Void is own
		}
	}

	//TODO:SHARE

	// assumes v is sorted.
	void check_params_from(const Slice<Parameter>& params, const Params& actual_from_params) {
		//TODO:PERF
		for (const Parameter& p : actual_from_params)
			if (!p.from)
				throw "todo"; //parameter should be marked 'from'
		for (const Parameter& p : params)
			if (!contains(actual_from_params, Ref<const Parameter>(&p)))
				throw "todo"; // Unnecessary 'from'
	}

	void check_return_effect(const FunSignature& sig, const ExprEffect& actual_effect) {
		Effect declared_return_effect = sig.effect;
		if (declared_return_effect == Effect::EOwn) {
			for (const Parameter& p : sig.parameters) {
				if (p.from)
					throw "Todo: param marked 'from' but we're not using it";
			}
			if (!actual_effect.is_own())
				throw "todo"; // Need to return a new instance of the type.
		} else {
			//Don't really care what they declared, since it only *reduces* the effect from a parameter.
			// A return effect inherits the effects of all parameters marked 'from'.
			if (actual_effect.is_own())
				throw "todo"; //return should be marked 'own'.
			if (!actual_effect.locals().empty()) throw "todo";
			check_params_from(sig.parameters, actual_effect.params());
		}
	}
}

void check_effects(FunDeclaration& fun) {
	switch (fun.body.kind()) {
		case AnyBody::Kind::Nil: unreachable();
		case AnyBody::Kind::CppSource:
			break;
		case AnyBody::Kind::Expr: {
			Ctx ctx { };
			check_return_effect(fun.signature, infer_effect(fun.body.expression(), ctx));
		}
	}
}

