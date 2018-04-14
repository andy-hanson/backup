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
	Option(const Option<T>& other) {
		*this = other;
	}
	Option(Option<T>&& other) : is_present(other.is_present) {
		if (is_present) {
			storage.value = std::move(other.storage.value);
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

	void operator=(Option<T> other) {
		is_present = other.is_present;
		if (is_present) {
			storage.value = other.storage.value;
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

template <typename T, typename U>
struct OptionMap {
	const Option<T>& op;

	template <typename Cb>
	Option<U> operator()(Cb cb) const {
		return op ? Option<U>(cb(op.get())) : Option<U>();
	}
};

template <typename T, typename U>
OptionMap<T, U> map(const Option<T>& op) {
	return { op };
}
