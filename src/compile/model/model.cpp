#include "model.h"

#include "../../util/collection_util.h"
#include "../../util/hash_util.h"

void StructBody::operator=(const StructBody& other) {
	_kind = other._kind;
	switch (_kind) {
		case Kind::Nil: break;
		case Kind::Fields:
			data.fields = other.data.fields;
			break;
		case Kind::CppName:
			data.cpp_name = other.data.cpp_name;
			break;
	}
}

void Type::operator=(const Type& other) {
	_kind = other._kind;
	switch (other._kind) {
		case Kind::Nil:
		case Kind::Bogus:
			break;
		case Kind::InstStruct: data.inst_struct = other.data.inst_struct; break;
		case Kind::Param: data.param = other.data.param; break;
	}
}

void AnyBody::operator=(const AnyBody& other) {
	_kind = other._kind;
	switch (other._kind) {
		case Kind::Nil:
			break;
		case Kind::Expr:
			data.expression = other.data.expression;
			break;
		case Kind::CppSource:
			data.cpp_source = other.data.cpp_source;
			break;
	}
}

SpecUse::SpecUse(Ref<const SpecDeclaration> _spec, Slice<Type> _type_arguments) : spec(_spec), type_arguments(_type_arguments) {
	assert(spec->type_parameters.size() == type_arguments.size());
}

bool InstStruct::is_deeply_concrete() const {
	return every(type_arguments, [](const Type& t) {  return t.is_inst_struct() && t.inst_struct().is_deeply_concrete(); });
}

hash_t InstStruct::hash::operator()(const InstStruct& i) {
	return hash_combine(Ref<const StructDeclaration>::hash{}(i.strukt), hash_arr(i.type_arguments, Type::hash {}));
}

bool operator==(const InstStruct& a, const InstStruct& b) {
	return a.strukt == b.strukt && a.type_arguments == b.type_arguments;
}

hash_t Type::hash::operator()(const Type& t) const {
	switch (t.kind()) {
		case Type::Kind::Nil: unreachable();
		case Type::Kind::Bogus:
			todo();
		case Type::Kind::Param:
			todo();
		case Type::Kind::InstStruct:
			return InstStruct::hash{}(t.inst_struct());
	}
}

bool operator==(const Type& a, const Type& b) {
	if (a.kind() != b.kind()) return false;
	switch (a.kind()) {
		case Type::Kind::Nil: unreachable();
		case Type::Kind::Bogus: return true;
		case Type::Kind::InstStruct: return a.inst_struct() == b.inst_struct();
		case Type::Kind::Param: return a.param() == b.param();
	}
}

bool FunSignature::is_generic() const {
	return !type_parameters.is_empty() || !specs.is_empty();
}
