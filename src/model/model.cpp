#include "model.h"

StringSlice effect_name(Effect e) {
	switch (e) {
		case Effect::Pure: return "pure";
		case Effect::Get: return "get";
		case Effect::Set: return "set";
		case Effect::Io: return "io";
	}
}

SpecUse::SpecUse(ref<const SpecDeclaration> _spec, DynArray<Type> _type_arguments) : spec(_spec), type_arguments(_type_arguments) {
	assert(spec->type_parameters.size() == type_arguments.size());
}

bool PlainType::is_deeply_plain() const {
	return every(inst_struct.type_arguments, [](const Type& t) {  return t.is_plain() && t.plain().is_deeply_plain(); });
}

bool operator==(const InstStruct& a, const InstStruct& b) {
	return a.strukt == b.strukt && a.type_arguments == b.type_arguments;
}

bool operator==(const PlainType& a, const PlainType& b) {
	return a.effect == b.effect && a.inst_struct == b.inst_struct;
}

bool operator==(const Type& a, const Type& b) {
	if (a.kind() != b.kind()) return false;
	switch (a.kind()) {
		case Type::Kind::Nil: assert(false);
		case Type::Kind::Plain: return a.plain() == b.plain();
		case Type::Kind::Param: return a.param() == b.param();
	}
}
