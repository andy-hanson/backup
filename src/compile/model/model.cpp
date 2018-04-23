#include "model.h"

#include "../../util/collection_util.h"

SpecUse::SpecUse(ref<const SpecDeclaration> _spec, Arr<Type> _type_arguments) : spec(_spec), type_arguments(_type_arguments) {
	assert(spec->type_parameters.size() == type_arguments.size());
}

bool InstStruct::is_deeply_concrete() const {
	return every(type_arguments, [](const Type& t) {  return t.is_inst_struct() && t.inst_struct().is_deeply_concrete(); });
}

bool operator==(const InstStruct& a, const InstStruct& b) {
	return a.strukt == b.strukt && a.type_arguments == b.type_arguments;
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
