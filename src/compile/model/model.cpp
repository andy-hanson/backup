#include "model.h"

#include "../../util/store/collection_util.h" // every
#include "../../util/store/slice_util.h" // ==
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

SpecUse::SpecUse(Ref<const SpecDeclaration> _spec, Slice<Type> _type_arguments) : spec{_spec}, type_arguments{_type_arguments} {
	assert(spec->type_parameters.size() == type_arguments.size());
}

bool InstStruct::is_deeply_concrete() const {
	return every(type_arguments, [](const Type& t) {  return t.stored_type().is_inst_struct() && t.stored_type().inst_struct().is_deeply_concrete(); });
}

void StoredType::operator=(const StoredType& other) {
	_kind = other._kind;
	switch (other._kind) {
		case Kind::Nil:
		case Kind::Bogus:
			break;
		case Kind::InstStruct: data.inst_struct = other.data.inst_struct; break;
		case Kind::TypeParameter: data.param = other.data.param; break;
	}
}

bool FunSignature::is_generic() const {
	return !type_parameters.is_empty() || !specs.is_empty();
}
