#include "Names.h"

#include "./mangle.h"

namespace {
	void write_type_for_fun_name(Arena::StringBuilder& sb, const InstStruct& i) {
		sb << mangle { i.strukt->name };
		if (!i.type_arguments.empty()) throw "todo";
	}

	void write_type_for_fun_name(Arena::StringBuilder& sb, const Type& type) {
		switch (type.kind()) {
			case Type::Kind::Nil:
			case Type::Kind::Bogus:
				assert(false);
			case Type::Kind::Param:
				sb << mangle { type.param()->name };
				break;
			case Type::Kind::InstStruct:
				write_type_for_fun_name(sb, type.inst_struct());
		}
	}

	ArenaString escape_struct_name(const Identifier& module_name, const Identifier& struct_name, Arena& arena, bool is_multiple_with_same_name) {
		Arena::StringBuilder sb = arena.string_builder(100);
		sb << mangle{struct_name};
		if (is_multiple_with_same_name)
			sb << '_' << mangle{module_name};
		return sb.finish();
	}

	ArenaString escape_field_name(const Identifier& field_name, Arena& arena) {
		Arena::StringBuilder sb = arena.string_builder(100);
		sb << mangle{field_name};
		return sb.finish();
	}

	class FunIds {
		// Filled lazily, because we won't need ids for most funs.
		Map<ref<const ConcreteFun>, uint, ref<const ConcreteFun>::hash> ids;
		uint next_id = 1;

	public:
		uint get_id(ref<const ConcreteFun> f) {
			uint& i = ids.get_or_create(f);
			if (i == 0) {
				i = next_id;
				++next_id;
			}
			return i;
		}
	};

	const StringSlice OVERLOAD = "_overload_";
	const StringSlice INST = "_inst";

	ArenaString escape_fun_name(const ConcreteFun& f, bool is_overloaded, bool is_instantiated, FunIds& ids, Arena& arena) {
		Arena::StringBuilder sb = arena.string_builder(100);
		const FunSignature& sig = f.fun_declaration->signature;
		sb << mangle{sig.name};
		if (is_overloaded) {
			sb << OVERLOAD << mangle {f.fun_declaration->containing_module->name()};
			write_type_for_fun_name(sb, sig.return_type);
			for (const Parameter& p : sig.parameters) {
				sb << '_';
				write_type_for_fun_name(sb, p.type);
			}
		}
		if (is_instantiated) {
			sb << INST;
			for (const InstStruct& i : f.type_arguments) {
				sb << '_';
				write_type_for_fun_name(sb, i);
			}
			for (const Arr<ref<const ConcreteFun>>& spec_impl : f.spec_impls)
				for (ref<const ConcreteFun> c : spec_impl)
					sb << '_' << ids.get_id(c);
		}
		return sb.finish();
	}

	class DuplicateNamesGetter {
		Set<Identifier, Identifier::hash> seen;
		Set<Identifier, Identifier::hash> duplicates;

	public:
		void add(const Identifier& i) {
			if (!seen.insert(i).was_added)
				duplicates.insert(i);
		}

		bool has_duplicate(const Identifier& i) {
			return duplicates.has(i);
		}
	};
}

Names get_names(const Arr<Module>& modules, const FunInstantiations& fun_instantiations, Arena& arena) {
	Set<Identifier, Identifier::hash> all_module_names;
	DuplicateNamesGetter all_struct_names;
	DuplicateNamesGetter all_fun_names;

	for (const Module& module : modules) {
		for (const StructDeclaration& strukt : module.structs_declaration_order)
			all_struct_names.add(strukt.name);
		for (const FunDeclaration& f : module.funs_declaration_order)
			all_fun_names.add(f.name());
		all_module_names.must_insert(module.name()); // TODO: if there are two modules with the same name, need to improve escaping
	}

	FunIds ids;
	Names names;

	for (const Module& module : modules) {
		for (const StructDeclaration& strukt : module.structs_declaration_order) {
			names.struct_names.must_insert(&strukt, escape_struct_name(strukt.containing_module->name(), strukt.name, arena, all_struct_names.has_duplicate(strukt.name)));
			if (strukt.body.is_fields())
				for (const StructField& field : strukt.body.fields())
					names.field_names.must_insert(&field, escape_field_name(field.name, arena));
		}

		for (const FunDeclaration& f : module.funs_declaration_order) {
			const Set<ConcreteFun, ConcreteFun::hash>& instances = fun_instantiations.must_get(&f);
			bool is_overloaded = all_fun_names.has_duplicate(f.name());
			bool is_instantiated = instances.size() != 1;
			for (const ConcreteFun& cf : instances)
				names.fun_names.must_insert(&cf, escape_fun_name(cf, is_overloaded, is_instantiated, ids, arena));
		}
	}

	return names;
}
