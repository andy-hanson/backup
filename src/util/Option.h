#pragma once

#include <cassert>

#include "ptr.h"

template <typename T>
class Option {
	static_assert(!std::is_pointer<T>::value, "Use opt_ref instead");

	bool is_present;
	union OptionStorage {
		char dummy __attribute__((unused));
		T value;

		OptionStorage() {} // leave uninitialized
		~OptionStorage() {}
	};
	OptionStorage storage;

public:
	Option() : is_present(false) {}
	Option(const Option<T>& other) : is_present(other.is_present) { storage.value = other.storage.value; }
	void operator=(Option<T> other) {
		is_present = other.is_present;
		if (is_present) {
			storage.value = other.storage.value;
		}
	}
	Option(T _value) : is_present(true) {
		storage.value = _value;
	}
	~Option() {
		if (is_present) {
			storage.value.~T();
		}
	}

	void operator=(T value) {
		is_present = true;
		storage.value = value;
	}

	operator bool() const {
		return is_present;
	}

	const T& get() const {
		assert(is_present);
		return storage.value;
	}

	Option<const T&> as_ref() const {
		return is_present ? Option<const T&> { get() } : Option<const T&> {};
	}
};

template <typename T>
class Option<T&> {
	T* ref;

public:
	Option() : ref(nullptr) {}
	Option(T& value) : ref(&value) {}

	operator bool() const {
		return ref != nullptr;
	}

	const T& get() const {
		assert(ref != nullptr);
		return *ref;
	}

	const T& or_else(const T& elze) const {
		return ref == nullptr ? elze : get();
	}
};

template <typename Out>
struct OptionMap {
	template <typename In, typename /*In => Out*/ Cb>
	Option<Out> operator()(const Option<In>& in, Cb cb) const {
		return in ? Option<Out>{cb(in.get())} : Option<Out>{};
	}
};

template <typename Out>
OptionMap<Out> map() { return {}; }

template <typename T>
Option<ref<T>> un_ref(const Option<const ref<T>&> in) {
	return in ? Option<ref<T>>{in.get()} : Option<ref<T>>{};
}
template <typename T>
Option<ref<T>> to_ref(const Option<T&> in) {
	return in ? Option<ref<T>>{&in.get()} : Option<ref<T>>{};
}

template <typename Out>
struct MapOp {
	template <typename In, typename /*Option<In> => Option<Out>*/ Cb>
	Option<Out> operator()(const Option<In>& in, Cb cb) const {
		return in ? cb(in.get()) : Option<Out>{};
	}
};
template <typename Out>
MapOp<Out> map_op() { return {}; };

template <typename T, typename /*() => Option<T>*/ Cb>
Option<T> or_option(const Option<T>& op, Cb cb) {
	return op ? op : cb();
}
