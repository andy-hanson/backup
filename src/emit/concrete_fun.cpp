#include "concrete_fun.h"

#include "../parse/type_utils.h"

#include "../util/collection_util.h"
#include "../util/Map.h"

namespace {
	template<typename K, typename V>
	Added<V> add_to_map_of_sets(Map<K, Sett<V>>& map, K key, V value) {
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

	ConcreteFun get_concrete_called(const ConcreteFun& fun, const Called& called, Arena& scratch_arena) {
		DynArray<PlainType> called_type_arguments = scratch_arena.map_array<Type, PlainType>(called.type_arguments)([&](const Type& type_argument) {
			return substitute_type_arguments(type_argument, fun, scratch_arena);
		});

		DynArray<ref<const ConcreteFun>> called_specs = scratch_arena.map_array<Called, ref<const ConcreteFun>>(called.spec_impls)([](const Called& called_spec __attribute__((unused))) -> ref<const ConcreteFun> {
			//NOTE: the spec instantiations may be additional ConcreteFun_s !

			//Added<ConcreteFun> a = add_to_map_of_sets(map, ConcreteFun { })
			//Use fun->spec_impls;
			throw "Todo";
		});

		switch (called.called_declaration.kind()) {
			case CalledDeclaration::Kind::Spec:
				// If the thing being called was a Spec, we have to match it against the current ConcreteFun.
				called.called_declaration.spec();
				throw "todo";
			case CalledDeclaration::Kind::Fun:
				//Called a FunDeclaration directly, easy.
				return ConcreteFun { called.called_declaration.fun(), called_type_arguments, called_specs };
		}
	}
}

EveryConcreteFun get_every_concrete_fun(const std::vector<ref<Module>>& modules, Arena& scratch_arena) {
	EveryConcreteFun res;
	// Stack of ConcreteFun_s whose bodies we need to analyze for references.
	std::vector<ref<const ConcreteFun>> to_analyze;

	for (ref<const Module> m : modules) {
		for (ref<const FunDeclaration> f : m->funs_declaration_order) {
			if (!f->signature.is_generic()) {
				Added<ConcreteFun> a = add_to_map_of_sets(res.fun_instantiations, f, ConcreteFun { f, {}, {}});
				if (a.was_added) to_analyze.push_back(a.value);
			}
		}
	}

	while (!to_analyze.empty()) {
		ref<const ConcreteFun> fun = to_analyze.back();
		to_analyze.pop_back();

		const AnyBody& body = fun->fun_declaration->body;
		if (body.kind() != AnyBody::Kind::Expression) continue;
		each_dependent_fun(body.expression(), [&](ref<const Called> called) {
			ConcreteFun concrete_called = get_concrete_called(*fun, called, scratch_arena);
			Added<ConcreteFun> added = add_to_map_of_sets(res.fun_instantiations, concrete_called.fun_declaration, concrete_called);
			if (added.was_added) to_analyze.push_back(added.value);
			res.resolved_calls.must_insert({ fun, called }, added.value );
		});
	}

	return res;
}
