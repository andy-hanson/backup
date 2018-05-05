#include "concrete_fun.h"

#include "../util/hash_util.h"

namespace {
	// Recursively replaces every type parameter with a corresponding type argument.
	InstStruct substitute_type_arguments(const TypeParameter& t, const Slice<TypeParameter>& type_parameters, const Slice<InstStruct>& type_arguments) {
		assert(&type_parameters[t.index] == &t);
		return type_arguments[t.index];
	}

	InstStruct substitute_type_arguments(const Type& t, const Slice<TypeParameter>& params, const Slice<InstStruct>& args, Arena& arena) {
		switch (t.kind()) {
			case Type::Kind::Nil:
			case Type::Kind::Bogus:
				// This should only be called from emit, and we don't emit if there were compile errors.
				unreachable();
			case Type::Kind::Param:
				return substitute_type_arguments(t.param(), params, args);
			case Type::Kind::InstStruct: {
				const InstStruct& i = t.inst_struct();
				return some(i.type_arguments, [](const Type& ta) { return ta.is_parameter(); })
					? InstStruct { i.strukt, map<Type>()(arena, i.type_arguments, [&](const Type& ta) { return Type{substitute_type_arguments(ta, params, args, arena)}; }) }
					: i;
			}
		}
	}

	template <typename V>
	struct InsertResult { bool was_added; Ref<const V> value; };
	template<uint capacity, typename K, typename V, typename KH>
	InsertResult<V> add_to_map_of_lists(MaxSizeMap<capacity, K, NonEmptyList<V>, KH>& map, K key, V value, Arena& arena) {
		//TODO:PERF avoid repeated map lookup
		if (!map.has(key)) {
			return { true, &map.must_insert(key, { value })->value.first() };
		} else {
			NonEmptyList<V>& list = map.must_get(key);
			Option<Ref<const V>> found = find(list, [&](const V& v) { return v == value; });
			if (found.has())
				return { false, found.get() };
			else {
				list.prepend(value, arena);
				return { true, &list.first() };
			}
		}
	}

	// Iterates over every Called in the body.
	template<typename Cb>
	void each_dependent_fun(const Expression& body, Cb cb) {
		MaxSizeVector<16, Ref<const Expression>> stack;
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
					unreachable();
			}
		} while (!stack.empty());
	}

	ConcreteFun get_concrete_called(const ConcreteFun& calling_fun, const Called& called, const EveryConcreteFun& res, Arena& scratch_arena) {
		Slice<InstStruct> called_type_arguments = map<InstStruct>()(scratch_arena, called.type_arguments, [&](const Type& type_argument) {
			return substitute_type_arguments(type_argument, calling_fun, scratch_arena);
		});

		//NOTE: currently, the function that matches a spec must be an exact match, not an instantiation of some generic function. So no recursive instantiations to worry about.
		Slice<Slice<Ref<const ConcreteFun>>> concrete_spec_impls =
		map<Slice<Ref<const ConcreteFun>>>()(scratch_arena, called.spec_impls, [&](const Slice<CalledDeclaration>& called_specs) {
			return map<Ref<const ConcreteFun>>()(scratch_arena, called_specs, [&](const CalledDeclaration& called_spec) {
				switch (called_spec.kind()) {
					case CalledDeclaration::Kind::Spec:
						throw "todo";
					case CalledDeclaration::Kind::Fun: {
						Ref<const FunDeclaration> spec_impl = called_spec.fun();
						if (spec_impl->signature.is_generic()) throw "todo";
						// Since it's non-generic, should have exactly 1 instantiation.
						const NonEmptyList<ConcreteFun>& list = res.fun_instantiations.get(spec_impl).get();
						assert(!list.has_more_than_one());
						return &list.first();
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

ConcreteFun::ConcreteFun(Ref<const FunDeclaration> _fun_declaration, Slice<InstStruct> _type_arguments, Slice<Slice<Ref<const ConcreteFun>>> _spec_impls)
	: fun_declaration(_fun_declaration), type_arguments(_type_arguments), spec_impls(_spec_impls) {
	assert(fun_declaration->signature.type_parameters.size() == type_arguments.size());
	assert(fun_declaration->signature.specs.size() == spec_impls.size());
	assert(each_corresponds(fun_declaration->signature.specs, spec_impls, [](const SpecUse& spec_use, const Slice<Ref<const ConcreteFun>>& sig_impls) {
		return spec_use.spec->signatures.size() == sig_impls.size();
	}));
	assert(every(type_arguments, [](const InstStruct& p) { return p.is_deeply_concrete(); }));
}

hash_t ConcreteFun::hash::operator()(const ConcreteFun& c) const {
	// Don't hash the spec_impls because that could lead to infinite recursion.
	return hash_combine(Ref<const FunDeclaration>::hash{}(c.fun_declaration), hash_arr(c.type_arguments, InstStruct::hash {}));
}

bool operator==(const ConcreteFun& a, const ConcreteFun& b) {
	return a.fun_declaration == b.fun_declaration && a.type_arguments == b.type_arguments && a.spec_impls == b.spec_impls;
}

hash_t ConcreteFunAndCalled::hash::operator()(const ConcreteFunAndCalled& c) const {
	return hash_combine(Ref<const ConcreteFun>::hash{}(c.fun), Ref<const Called>::hash{}(c.called));
}

bool operator==(const ConcreteFunAndCalled& a, const ConcreteFunAndCalled& b) {
	return a.fun == b.fun && a.called == b.called;
}

InstStruct substitute_type_arguments(const Type& type_argument, const ConcreteFun& fun, Arena& arena) {
	return substitute_type_arguments(type_argument, fun.fun_declaration->signature.type_parameters, fun.type_arguments, arena);
}

EveryConcreteFun get_every_concrete_fun(const Slice<Module>& modules, Arena& scratch_arena) {
	EveryConcreteFun res;
	// Stack of ConcreteFun_s whose bodies we need to analyze for references.
	MaxSizeVector<16, Ref<const ConcreteFun>> to_analyze;

	for (const Module& m : modules) {
		for (const FunDeclaration& f : m.funs_declaration_order) {
			if (!f.signature.is_generic()) {
				InsertResult<ConcreteFun> a = add_to_map_of_lists(res.fun_instantiations, Ref<const FunDeclaration>{&f}, ConcreteFun { &f, {}, {}}, scratch_arena);
				if (a.was_added) to_analyze.push(a.value);
			}
		}
	}

	while (!to_analyze.empty()) {
		Ref<const ConcreteFun> fun = to_analyze.pop_and_return();
		const AnyBody& body = fun->fun_declaration->body;
		if (body.kind() != AnyBody::Kind::Expr) continue;
		each_dependent_fun(body.expression(), [&](Ref<const Called> called) {
			ConcreteFun concrete_called = get_concrete_called(*fun, called, res, scratch_arena);
			auto added = add_to_map_of_lists(res.fun_instantiations, concrete_called.fun_declaration, concrete_called, scratch_arena);
			if (added.was_added) to_analyze.push(added.value);
			res.resolved_calls.must_insert({ fun, called }, added.value );
		});
	}

	return res;
}
