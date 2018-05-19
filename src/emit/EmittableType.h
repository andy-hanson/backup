#pragma once

#include "../util/store/Map.h"
#include "../compile/model/model.h"

struct EmittableType;

class EmittableStruct {
public:
	Ref<const StructDeclaration> strukt;
	Slice<EmittableType> type_arguments;
	Slice<EmittableType> field_types;

private:
	friend class EmittableTypeCache;
	// Can't construct directly, must use EmittableTypeCache
	EmittableStruct(Ref<const StructDeclaration> _struct, Slice<EmittableType> _type_arguments, Slice<EmittableType> _field_types);
};

struct EmittableType {
	Ref<const EmittableStruct> inst_struct;
	bool is_pointer;

	struct hash {
		hash_t operator()(const EmittableType& e) const;
	};
};
inline bool operator==(const EmittableType& a, const EmittableType& b) {
	return a.inst_struct == b.inst_struct && a.is_pointer == b.is_pointer;
}
inline bool operator!=(const EmittableType& a, const EmittableType& b) {
	return !(a == b);
}

class EmittableTypeCache {
	Arena arena;
	Map<Ref<const StructDeclaration>, NonEmptyList<EmittableStruct>, Ref<const StructDeclaration>::hash> cache;

	Ref<const EmittableStruct> get_inst_struct(const InstStruct& inst_struct, const Slice<TypeParameter>& type_parameters, const Slice<EmittableType>& type_arguments);

public:
	inline EmittableTypeCache() : arena{}, cache{64, arena} {}

	EmittableType get_type(const Type& type, const Slice<TypeParameter>& type_parameters, const Slice<EmittableType>& type_arguments);

	inline Option<const NonEmptyList<EmittableStruct>&> get_types_for_struct(const StructDeclaration& decl) const {
		return cache.get(&decl);
	}

	template <typename /*const StructDeclaration&, const NonEmptyList<EmittableStruct>& => void*/ Cb>
	inline void each(Cb cb) const {
		cache.each(cb);
	}
};
