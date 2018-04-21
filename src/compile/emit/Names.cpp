#include "Names.h"

#include "./mangle.h"

namespace {
	void write_type_for_fun_name(Arena::StringBuilder& sb, const PlainType& type) {
		sb << mangle { type.inst_struct.strukt->name };
		if (!type.inst_struct.type_arguments.empty()) throw "todo";
	}

	void write_type_for_fun_name(Arena::StringBuilder& sb, const Type& type) {
		switch (type.kind()) {
			case Type::Kind::Nil:
				assert(false);
			case Type::Kind::Param:
				sb << mangle { type.param()->name };
				break;
			case Type::Kind::Plain:
				write_type_for_fun_name(sb, type.plain());
		}
	}

	ArenaString escape_struct_name(const Identifier& module_name, const Identifier& struct_name, Arena& arena, bool is_multiple_with_same_name) {
		Arena::StringBuilder sb = arena.string_builder(100);
		sb << mangle{struct_name};
		if (is_multiple_with_same_name)
			sb << '_' << mangle{module_name};
		return sb.finish();
	}

	class FunIds {
		// Filled lazily, because we won't need ids for most funs.
		Map<ref<const ConcreteFun>, uint> ids;
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

	ArenaString escape_fun_name(const ConcreteFun& f, bool is_overloaded, bool is_instantiated, FunIds& ids, Arena& arena) {
		Arena::StringBuilder sb = arena.string_builder(100);
		const FunSignature& sig = f.fun_declaration->signature;
		sb << mangle{sig.name};
		if (is_overloaded) {
			sb << "_overload_" << mangle {f.fun_declaration->containing_module->name};
			write_type_for_fun_name(sb, sig.return_type);
			for (const Parameter& p : sig.parameters) {
				sb << '_';
				write_type_for_fun_name(sb, p.type);
			}
		}
		if (is_instantiated) {
			sb << "_inst";
			for (const PlainType& t : f.type_arguments) {
				sb << '_';
				write_type_for_fun_name(sb, t);
			}
			for (const DynArray<ref<const ConcreteFun>>& spec_impl : f.spec_impls)
				for (ref<const ConcreteFun> c : spec_impl)
					sb << '_' << ids.get_id(c);
		}
		return sb.finish();
	}
}

Names get_names(const std::vector<ref<Module>>& modules, const FunInstantiations& fun_instantiations, Arena& arena) {
	Sett<Identifier> module_names;

	// Map from a name to all structs with that name.
	MultiMap<Identifier, ref<const StructDeclaration>> global_structs_table;

	for (ref<const Module> module : modules) {
		for (ref<const StructDeclaration> s : module->structs_declaration_order)
			global_structs_table.add(s->name, s);
		module_names.must_insert(module->name); //TODO: if 2 modules have the same name, harder to generate overload names
	}

	Names names;
	for (const auto& a : global_structs_table) {
		const Identifier& name = a.first;
		ref<const StructDeclaration> strukt = a.second;
		names.struct_names.must_insert(strukt, escape_struct_name(strukt->containing_module->name, name, arena, global_structs_table.count(a.first) != 1));
	}

	// Map from a name to all funs with that name.
	MultiMap<Identifier, ref<const Sett<ConcreteFun>>> global_funs_table;
	for (const auto& a : fun_instantiations) global_funs_table.add(a.first->name(), &a.second);

	FunIds ids;

	for (const auto& a : global_funs_table) {
		const Identifier& name = a.first;
		const Sett<ConcreteFun>& fn_instances = a.second;
		for (const ConcreteFun& f : fn_instances) {
			// If there are two separate fns with the same name, name them based on their *declaration*.
			// Then if there are two separate instantiations of the same fn, name them based on the types used to instantiate them.
			names.fun_names.must_insert(&f, escape_fun_name(f, /*is_overloaded*/ global_funs_table.count(name) > 1, /*is_instantiated*/ fn_instances.size() > 1, ids, arena));
		}
	}

	return names;
}
