#include "./Names.h"

namespace {
	Option<StringSlice> mangle_char(char c) {
		switch (c) {
			case '+':
				return Option<StringSlice> { "_add" };
			case '-':
				// '-' often used as a hyphen
				return Option<StringSlice> { "__" };
			case '*':
				return Option<StringSlice> { "_times" };
			case '/':
				return Option<StringSlice> { "_div" };
			case '<':
				return Option<StringSlice> { "_lt" };
			case '>':
				return Option<StringSlice> { "_gt" };
			case '=':
				return Option<StringSlice> { "_eq" };
			default:
				assert(('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'));
				return {};
		}
	}

	const StringSlice MAIN = "main";
	const StringSlice TRUE = "true";
	const StringSlice FALSE = "false";

	bool needs_special_mangle(const StringSlice& name) {
		return name == MAIN || name == TRUE || name == FALSE;
	}

	bool needs_mangle(const StringSlice& name) {
		return needs_special_mangle(name) || some(name, [](char c) { return mangle_char(c).has(); });
	}

	template <typename WriterLike>
	void write_mangled(WriterLike& out, const StringSlice& name) {
		if (needs_special_mangle(name))
			out << '_' << name;
		else {
			for (char c : name) {
				auto m = mangle_char(c);
				if (m.has())
					out << m.get();
				else
					out << c;
			}
		}
	}

	ArenaString mangled(const StringSlice& name, Arena& out) {
		assert(needs_mangle(name));
		StringBuilder sb { out, name.size() * 2 };
		write_mangled(sb, name);
		return sb.finish();
	}

	ArenaString mangled(const StringSlice& name, uint id, Arena& out) {
		StringBuilder sb { out, name.size() * 2 };
		write_mangled(sb, name);
		sb << id;
		return sb.finish();
	}
}

Names get_names(const EmittableTypeCache& types, const ConcreteFunsCache& funs, Arena& out_arena) {
	Names names { { 32, out_arena }, { 32, out_arena }, { 32, out_arena } };

	types.each([&](const StructDeclaration& strukt, const NonEmptyList<EmittableStruct> emittables) {
		if (emittables.has_more_than_one()) {
			uint id = 0;
			for (const EmittableStruct& e : emittables) {
				names.struct_names.must_insert(&e, mangled(strukt.name, id, out_arena));
				++id;
			}
		} else if (needs_mangle(strukt.name))
			names.struct_names.must_insert(&emittables.only(), mangled(strukt.name, out_arena));

		if (strukt.body.is_fields())
			for (const StructField& f : strukt.body.fields())
				if (needs_mangle(f.name))
					names.field_names.must_insert(&f, mangled(f.name, out_arena));
	});

	funs.each([&](const FunDeclaration& fun, const NonEmptyList<ConcreteFun>& concretes) {
		const StringSlice& name = fun.name();
		if (concretes.has_more_than_one()) {
			uint id = 0;
			for (const ConcreteFun& cf : concretes) {
				names.fun_names.must_insert(&cf, mangled(name, id, out_arena));
				++id;
			}
		} else if (needs_mangle(name)) {
			names.fun_names.must_insert(&concretes.only(), mangled(name, out_arena));
		}
	});

	return names;
}

Writer& operator<<(Writer& out, const Names::StructNameWriter& s) {
	Option<const ArenaString&> name = s.names.struct_names.get(&s.strukt);
	return out << (name.has() ? name.get() : s.strukt.strukt->name);
}

Writer& operator<<(Writer& out, const Names::FieldNameWriter& s) {
	Option<const ArenaString&> name = s.names.field_names.get(&s.field);
	return out << (name.has() ? name.get() : s.field.name);
}

Writer& operator<<(Writer& out, const Names::FunNameWriter& f) {
	Option<const ArenaString&> name = f.names.fun_names.get(&f.fun);
	return out << (name.has() ? name.get() : f.fun.fun_declaration->name());
}

Writer& operator<<(Writer& out, const Names::ParameterNameWriter& p) {
	write_mangled(out, p.parameter.name);
	return out;
}
