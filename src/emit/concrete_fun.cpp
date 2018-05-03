#include "concrete_fun.h"

#include "../util/hash_util.h"

namespace {
	// Recursively replaces every type parameter with a corresponding type argument.
	InstStruct substitute_type_arguments(const TypeParameter& t, const Arr<TypeParameter>& type_parameters, const Arr<InstStruct>& type_arguments) {
		assert(&type_parameters[t.index] == &t);
		return type_arguments[t.index];
	}

	InstStruct substitute_type_arguments(const Type& t, const Arr<TypeParameter>& params, const Arr<InstStruct>& args, Arena& arena) {
		switch (t.kind()) {
			case Type::Kind::Nil:
			case Type::Kind::Bogus:
				// This should only be called from emit, and we don't emit if there were compile errors.
				assert(false);
			case Type::Kind::Param:
				return substitute_type_arguments(t.param(), params, args);
			case Type::Kind::InstStruct: {
				const InstStruct& i = t.inst_struct();
				return some(i.type_arguments, [](const Type& ta) { return ta.is_parameter(); })
					? InstStruct { i.strukt, arena.map<Type>()(i.type_arguments, [&](const Type& ta) { return Type{substitute_type_arguments(ta, params, args, arena)}; }) }
					: i;
			}
		}
	}

	template<typename K, typename V, typename KH, typename VH>
	typename HeapAllocatedSet<V, VH>::InsertResult add_to_map_of_sets(HeapAllocatedMap<K, HeapAllocatedSet<V, VH>, KH>& map, K key, V value) {
		return map.get_or_create(key).insert(value);
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
				case Expression::Kind::Assert:
					stack.push(&e.asserted());
					break;
				case Expression::Kind::Pass:
					break;
				case Expression::Kind::Nil:
				case Expression::Kind::Bogus:
					assert(false);
			}
		} while (!stack.empty());
	}

	ConcreteFun get_concrete_called(const ConcreteFun& calling_fun, const Called& called, const EveryConcreteFun& res, Arena& scratch_arena) {
		Arr<InstStruct> called_type_arguments = scratch_arena.map<InstStruct>()(called.type_arguments, [&](const Type& type_argument) {
			return substitute_type_arguments(type_argument, calling_fun, scratch_arena);
		});

		//NOTE: currently, the function that matches a spec must be an exact match, not an instantiation of some generic function. So no recursive instantiations to worry about.
		Arr<Arr<ref<const ConcreteFun>>> concrete_spec_impls =
		scratch_arena.map<Arr<ref<const ConcreteFun>>>()(called.spec_impls, [&](const Arr<CalledDeclaration>& called_specs) {
			return scratch_arena.map<ref<const ConcreteFun>>()(called_specs, [&](const CalledDeclaration& called_spec) {
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

ConcreteFun::ConcreteFun(ref<const FunDeclaration> _fun_declaration, Arr<InstStruct> _type_arguments, Arr<Arr<ref<const ConcreteFun>>> _spec_impls)
	: fun_declaration(_fun_declaration), type_arguments(_type_arguments), spec_impls(_spec_impls) {
	assert(fun_declaration->signature.type_parameters.size() == type_arguments.size());
	assert(fun_declaration->signature.specs.size() == spec_impls.size());
	assert(each_corresponds(fun_declaration->signature.specs, spec_impls, [](const SpecUse& spec_use, const Arr<ref<const ConcreteFun>>& sig_impls) {
		return spec_use.spec->signatures.size() == sig_impls.size();
	}));
	assert(every(type_arguments, [](const InstStruct& p) { return p.is_deeply_concrete(); }));
}

size_t ConcreteFun::hash::operator()(const ConcreteFun& c) const {
	// Don't hash the spec_impls because that could lead to infinite recursion.
	return hash_combine(ref<const FunDeclaration>::hash{}(c.fun_declaration), hash_arr(c.type_arguments, InstStruct::hash {}));
}

bool operator==(const ConcreteFun& a, const ConcreteFun& b) {
	return a.fun_declaration == b.fun_declaration && a.type_arguments == b.type_arguments && a.spec_impls == b.spec_impls;
}

size_t ConcreteFunAndCalled::hash::operator()(const ConcreteFunAndCalled& c) const {
	return hash_combine(ref<const ConcreteFun>::hash{}(c.fun), ref<const Called>::hash{}(c.called));
}

bool operator==(const ConcreteFunAndCalled& a, const ConcreteFunAndCalled& b) {
	return a.fun == b.fun && a.called == b.called;
}

InstStruct substitute_type_arguments(const Type& type_argument, const ConcreteFun& fun, Arena& arena) {
	return substitute_type_arguments(type_argument, fun.fun_declaration->signature.type_parameters, fun.type_arguments, arena);
}

EveryConcreteFun get_every_concrete_fun(const Arr<Module>& modules, Arena& scratch_arena) {
	EveryConcreteFun res;
	// Stack of ConcreteFun_s whose bodies we need to analyze for references.
	MaxSizeVector<16, ref<const ConcreteFun>> to_analyze;

	for (const Module& m : modules) {
		for (const FunDeclaration& f : m.funs_declaration_order) {
			if (!f.signature.is_generic()) {
				auto a = add_to_map_of_sets(res.fun_instantiations, ref<const FunDeclaration>{&f}, ConcreteFun { &f, {}, {}});
				if (a.was_added) to_analyze.push(a.value);
			}
		}
	}

	while (!to_analyze.empty()) {
		ref<const ConcreteFun> fun = to_analyze.pop_and_return();
		const AnyBody& body = fun->fun_declaration->body;
		if (body.kind() != AnyBody::Kind::Expr) continue;
		each_dependent_fun(body.expression(), [&](ref<const Called> called) {
			ConcreteFun concrete_called = get_concrete_called(*fun, called, res, scratch_arena);
			auto added = add_to_map_of_sets(res.fun_instantiations, concrete_called.fun_declaration, concrete_called);
			if (added.was_added) to_analyze.push(added.value);
			res.resolved_calls.must_insert({ fun, called }, added.value );
		});
	}

	return res;
}
