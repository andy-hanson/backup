#include "concrete_fun.h"

#include "../../util/collection_util.h"
#include "../../util/Map.h"

namespace {
	template<typename K, typename V>
	InsertResult<V> add_to_map_of_sets(Map<K, Sett<V>>& map, K key, V value) {
		Sett<V>& set = map.get_or_create(key);
		return set.insert(value);
	}

	// Iterates over every Called in the body.
	template<typename Cb>
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
					cb(&c.called);
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

	ConcreteFun get_concrete_called(const ConcreteFun& calling_fun, const Called& called, const EveryConcreteFun& res, Arena& scratch_arena) {
		DynArray<PlainType> called_type_arguments = scratch_arena.map_array<PlainType>()(called.type_arguments, [&](const Type& type_argument) {
			return substitute_type_arguments(type_argument, calling_fun, scratch_arena);
		});

		//NOTE: currently, the function that matches a spec must be an exact match, not an instantiation of some generic function. So no recursive instantiations to worry about.
		DynArray<DynArray<ref<const ConcreteFun>>> concrete_spec_impls =
			scratch_arena.map_array<DynArray<ref<const ConcreteFun>>>()(called.spec_impls, [&](const DynArray<CalledDeclaration>& called_specs) {
			return scratch_arena.map_array<ref<const ConcreteFun>>()(called_specs, [&](const CalledDeclaration& called_spec) {
				switch (called_spec.kind()) {
					case CalledDeclaration::Kind::Spec:
						throw "todo";
					case CalledDeclaration::Kind::Fun: {
						ref<const FunDeclaration> spec_impl = called_spec.fun();
						if (spec_impl->signature.is_generic()) throw "todo";
						// Since it's non-generic, should have exactly 1 instantiation.
						return &res.fun_instantiations.get(spec_impl).get().only();
					}
				}
			});
		});

		switch (called.called_declaration.kind()) {
			case CalledDeclaration::Kind::Spec: {
				// Get the corresponding spec_impl.
				const SpecUseSig& s = called.called_declaration.spec();
				uint spec_index = get_index(calling_fun.fun_declaration->signature.specs, s.spec_use);
				uint sig_index = get_index(s.spec_use->spec->signatures, s.signature);
				return calling_fun.spec_impls[spec_index][sig_index];
			}
			case CalledDeclaration::Kind::Fun:
				//Called a FunDeclaration directly, easy.
				return ConcreteFun { called.called_declaration.fun(), called_type_arguments, concrete_spec_impls };
		}
	}
}

EveryConcreteFun get_every_concrete_fun(const Vec<ref<Module>>& modules, Arena& scratch_arena) {
	EveryConcreteFun res;
	// Stack of ConcreteFun_s whose bodies we need to analyze for references.
	Vec<ref<const ConcreteFun>> to_analyze;

	for (ref<const Module> m : modules) {
		for (ref<const FunDeclaration> f : m->funs_declaration_order) {
			if (!f->signature.is_generic()) {
				InsertResult<ConcreteFun> a = add_to_map_of_sets(res.fun_instantiations, f, ConcreteFun { f, {}, {}});
				if (a.was_added) to_analyze.push_back(a.value);
			}
		}
	}

	while (!to_analyze.empty()) {
		ref<const ConcreteFun> fun = to_analyze.pop();
		const AnyBody& body = fun->fun_declaration->body;
		if (body.kind() != AnyBody::Kind::Expr) continue;
		each_dependent_fun(body.expression(), [&](ref<const Called> called) {
			ConcreteFun concrete_called = get_concrete_called(*fun, called, res, scratch_arena);
			InsertResult<ConcreteFun> added = add_to_map_of_sets(res.fun_instantiations, concrete_called.fun_declaration, concrete_called);
			if (added.was_added) to_analyze.push_back(added.value);
			res.resolved_calls.must_insert({ fun, called }, added.value );
		});
	}

	return res;
}
