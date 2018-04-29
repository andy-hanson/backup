#include "model.h"

#include "../../util/collection_util.h"

SpecUse::SpecUse(ref<const SpecDeclaration> _spec, Arr<Type> _type_arguments) : spec(_spec), type_arguments(_type_arguments) {
	assert(spec->type_parameters.size() == type_arguments.size());
}

bool InstStruct::is_deeply_concrete() const {
	return every(type_arguments, [](const Type& t) {  return t.is_inst_struct() && t.inst_struct().is_deeply_concrete(); });
}

size_t InstStruct::hash::operator()(const InstStruct& i) {
	return hash_combine(ref<const StructDeclaration>::hash{}(i.strukt), hash_dyn_array(i.type_arguments, Type::hash{}));
}

bool operator==(const InstStruct& a, const InstStruct& b) {
	return a.strukt == b.strukt && a.type_arguments == b.type_arguments;
}

size_t Type::hash::operator()(const Type& t) const {
	switch (t.kind()) {
		case Type::Kind::Nil: assert(false);
		case Type::Kind::Bogus:
		case Type::Kind::Param:
			throw "todo";
		case Type::Kind::InstStruct:
			return InstStruct::hash{}(t.inst_struct());
	}
}

bool operator==(const Type& a, const Type& b) {
	if (a.kind() != b.kind()) return false;
	switch (a.kind()) {
		case Type::Kind::Nil: assert(false);
		case Type::Kind::Bogus: return true;
		case Type::Kind::InstStruct: return a.inst_struct() == b.inst_struct();
		case Type::Kind::Param: return a.param() == b.param();
	}
}

bool FunSignature::is_generic() const {
	return !type_parameters.empty() || !specs.empty();
}
