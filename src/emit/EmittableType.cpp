#include "./EmittableType.h"

#include "../util/hash_util.h"
#include "../util/store/map_of_lists_util.h"
#include "../util/store/slice_util.h" // ==

namespace {
	// Recursively replaces every type parameter with a corresponding type argument.
	EmittableType substitute_type_arguments(const TypeParameter& t, const Slice<TypeParameter>& type_parameters, const Slice<EmittableType>& type_arguments) {
		assert(&type_parameters[t.index] == &t);
		return type_arguments[t.index];
	}
}

EmittableStruct::EmittableStruct(Ref<const StructDeclaration> _struct, Slice<EmittableType> _type_arguments, Slice<EmittableType> _field_types)
	: strukt{_struct}, type_arguments{_type_arguments}, field_types{_field_types} {}

hash_t EmittableType::hash::operator()(const EmittableType& e) const {
	return hash_combine(Ref<const EmittableStruct>::hash{}(e.inst_struct), hash_bool(e.is_pointer));
}

Ref<const EmittableStruct> EmittableTypeCache::get_inst_struct(const InstStruct& inst_struct, const Slice<TypeParameter>& type_parameters, const Slice<EmittableType>& type_arguments) {
	Arena temp;
	// Allocated in temp arena because we'll probably use a cached result and not need this.
	Slice<EmittableType> temp_type_arguments = map<EmittableType>{}(temp, inst_struct.type_arguments, [&](const Type& t) {
		return get_type(t, type_parameters, type_arguments);
	});
	return add_to_map_of_lists(
		cache, inst_struct.strukt, arena,
		/*is_match*/ [&](const EmittableStruct& e) { return e.strukt == inst_struct.strukt && e.type_arguments == temp_type_arguments; },
		/*create_value*/ [&]() {
			const StructBody& body = inst_struct.strukt->body;
			Slice<EmittableType> struct_type_arguments = clone(temp_type_arguments, arena);
			Slice<EmittableType> field_types = body.kind() == StructBody::Kind::CppName ? Slice<EmittableType> {} : map<EmittableType>{}(arena, body.fields(), [&](const StructField& field) {
				return get_type(field.type, inst_struct.strukt->type_parameters, struct_type_arguments);
			});
			return EmittableStruct { inst_struct.strukt, struct_type_arguments, field_types };
		}
	).value;
}

EmittableType EmittableTypeCache::get_type(const Type& type, const Slice<TypeParameter>& type_parameters, const Slice<EmittableType>& type_arguments) {
	if (type.stored_type().is_type_parameter()) {
		if (type.lifetime().is_pointer()) todo();
		return substitute_type_arguments(type.stored_type().param(), type_parameters, type_arguments);
	} else
		return { get_inst_struct(type.stored_type().inst_struct(), type_parameters, type_arguments), type.lifetime().is_pointer() };
}


