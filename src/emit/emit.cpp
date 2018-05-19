#include "./emit.h"

#include "../util/store/Map.h"
#include "../compile/model/expr.h"
#include "./ConcreteFun.h"
#include "./emit_body.h"
#include "./emit_comment.h"
#include "./Names.h"
#include "./CAst_emit.h"

namespace {
	using Bodies = Map<Ref<const ConcreteFun>, CFunctionBody, Ref<const ConcreteFun>::hash>;

	void emit_bodies(Ref<const ConcreteFun> main, Bodies& bodies, const BuiltinTypes& builtin_types, ConcreteFunsCache& concrete_funs, EmittableTypeCache& types_cache, Arena& ast_arena) {
		MaxSizeVector<16, Ref<const ConcreteFun>> to_emit;
		to_emit.push(main);

		while (!to_emit.is_empty()) {
			Ref<const ConcreteFun> f = to_emit.pop_and_return();
			bodies.must_insert(f, emit_body(f, builtin_types, concrete_funs, types_cache, to_emit, ast_arena));
		}
	}

	template <typename /*EmittableStruct => void*/ Cb>
	void each_emittable_type(const Slice<Module>& modules, const EmittableTypeCache& types, Cb cb) {
		for (const Module& m : modules) {
			for (const StructDeclaration& s : m.structs_declaration_order) {
				Option<const NonEmptyList<EmittableStruct>&> o = types.get_types_for_struct(s);
				if (!o.has()) continue;
				for (const EmittableStruct& e : o.get())
					cb(e);
			}
		}
	}

	template<typename /*ConcreteFun => void*/ Cb>
	void each_concrete_fun(const Slice<Module>& modules, const ConcreteFunsCache& concrete_funs, Cb cb) {
		for (const Module& m : modules) {
			for (const FunDeclaration& f : m.funs_declaration_order) {
				Option<const NonEmptyList<ConcreteFun>&> o = concrete_funs.get_funs_for_declaration(f);
				if (!o.has()) continue;
				for (const ConcreteFun& cf : o.get()) {
					cb(cf);
				}
			}
		}
	}
}

Writer::Output emit(const Slice<Module>& modules, const BuiltinTypes& builtin_types, Arena& out_arena) {
	Arena temp;

	assert(!modules.is_empty());
	Option<Ref<const FunDeclaration>> main = find(modules[modules.size() - 1].funs_declaration_order, [&](const FunDeclaration& f) { return f.name() == "main"; });
	if (!main.has()) todo();

	ConcreteFunsCache concrete_funs;
	EmittableTypeCache types_cache;
	Bodies bodies { 64, temp };
	// Emitting function bodies will generate types and functions along the way.
	emit_bodies(concrete_funs.get_concrete_fun_for_main(main.get(), types_cache), bodies, builtin_types, concrete_funs, types_cache, temp);

	Names names = get_names(types_cache, concrete_funs, temp);

	Writer out { out_arena };
	out << "#include <assert.h>\n\n";

	// First write all structs
	each_emittable_type(modules, types_cache, [&](const EmittableStruct& e) {
		write_emittable_struct(out, e, names);
		out << Writer::nl << Writer::nl;
	});
	each_concrete_fun(modules, concrete_funs, [&](const ConcreteFun& f) {
		write_fun_header(out, f, names);
		out << ';' << Writer::nl;
	});
	out << Writer::nl;
	each_concrete_fun(modules, concrete_funs, [&](const ConcreteFun& f) {
		write_fun_implementation(out, f, bodies.must_get(&f), names);
		out << Writer::nl << Writer::nl;
	});

	out << "int main() { Void v; _main(&v); }\n";

	return out.finish();
}
