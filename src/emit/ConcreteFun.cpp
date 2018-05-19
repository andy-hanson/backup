#include "ConcreteFun.h"

#include "../util/store/slice_util.h" // get_index
#include "../util/hash_util.h"
#include "../compile/model/types_equal_ignore_lifetime.h"
#include "./substitute_type_arguments.h"

ConcreteFun::ConcreteFun(
	Ref<const FunDeclaration> _fun_declaration,
	Slice<EmittableType> _type_arguments,
	Slice<Slice<Ref<const ConcreteFun>>> _spec_impls,
	EmittableType _return_type,
	Slice<EmittableType> _parameter_types)
	: fun_declaration{_fun_declaration}, type_arguments{_type_arguments}, spec_impls{_spec_impls}, return_type{_return_type}, parameter_types{_parameter_types} {
	assert(fun_declaration->signature.type_parameters.size() == type_arguments.size());
	assert(fun_declaration->signature.specs.size() == spec_impls.size());
	assert(each_corresponds(fun_declaration->signature.specs, spec_impls, [](const SpecUse& spec_use, const Slice<Ref<const ConcreteFun>>& sig_impls) {
		return spec_use.spec->signatures.size() == sig_impls.size();
	}));
	assert(parameter_types.size() == fun_declaration->signature.arity());
}

Ref<const ConcreteFun> ConcreteFunsCache::get_concrete_fun_for_main(const FunDeclaration& main, EmittableTypeCache& type_cache) {
	const FunSignature& sig = main.signature;
	if (sig.is_generic()) todo(); //compile error
	auto get_type = [&](const Type& t) -> EmittableType { return type_cache.get_type(t, {}, {}); };
	EmittableType return_type = type_cache.get_type(sig.return_type, {}, {});
	Slice<EmittableType> parameter_types = map<EmittableType>{}(arena, sig.parameters, [&](const Parameter& p) { return get_type(p.type); });
	return &funs_map.must_insert(&main, NonEmptyList<ConcreteFun> { ConcreteFun { &main, {}, {}, return_type, parameter_types } }).value.first();
}

TryInsertResult<ConcreteFun> ConcreteFunsCache::get_concrete_fun_for_call(
	Ref<const ConcreteFun> current_concrete_fun, const Called& called, EmittableTypeCache& type_cache) {

	Ref<const FunDeclaration> called_fun = called.called_declaration.fun(); //TODO: handle spec calls
	const FunSignature& called_sig = called.called_declaration.sig();

	//TODO: don't always allocate this in out arena. Only if we need to insert it into the map.
	Arena temp;
	// Allocated in temp arena because we'll probably use a cached result and not need this.
	Slice<EmittableType> temp_type_arguments = map<EmittableType>{}(temp, called.type_arguments, [&](const Type& type_argument) {
		return type_cache.get_type(type_argument, current_concrete_fun->fun_declaration->signature.type_parameters, current_concrete_fun->type_arguments);
	});

	Slice<Slice<Ref<const ConcreteFun>>> temp_concrete_spec_impls = map<Slice<Ref<const ConcreteFun>>>{}(temp, called.spec_impls, [&](const Slice<CalledDeclaration>& called_specs) {
		return map<Ref<const ConcreteFun>>{}(arena, called_specs, [&](const CalledDeclaration& called_spec) {
			switch (called_spec.kind()) {
				case CalledDeclaration::Kind::Spec:
					todo();
				case CalledDeclaration::Kind::Fun: {
					Ref<const FunDeclaration> spec_impl = called_spec.fun();
					if (spec_impl->signature.is_generic()) todo();
					// Since it's non-generic, should have exactly 1 instantiation.
					const NonEmptyList<ConcreteFun>& list = funs_map.get(spec_impl).get();
					assert(!list.has_more_than_one());
					return &list.first();
				}
			}
		});
	});

	return add_to_map_of_lists(
		funs_map, called_fun, arena,
		/*is_match*/ [&](const ConcreteFun& cf) {
			return cf.fun_declaration == called_fun && cf.type_arguments == temp_type_arguments && cf.spec_impls == temp_concrete_spec_impls;
		},
		/*create_value*/ [&]() {
			auto get_type = [&](const Type& t) -> EmittableType {
				return type_cache.get_type(t, called_sig.type_parameters, temp_type_arguments);
			};
			Slice<EmittableType> type_arguments = clone(temp_type_arguments, arena);
			Slice<Slice<Ref<const ConcreteFun>>> concrete_spec_impls = clone(temp_concrete_spec_impls, arena);
			Slice<EmittableType> parameter_types = map<EmittableType> {}(arena, called_sig.parameters, [&](const Parameter& p) { return get_type(p.type); });
			return ConcreteFun { called_fun, type_arguments, concrete_spec_impls, get_type(called_sig.return_type), parameter_types };
		});
}
