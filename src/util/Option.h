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
	Option(const Option<T>& other) : is_present(other.is_present) { new(&storage.value) T(other.storage.value); }
	void operator=(Option<T> other) {
		bool was_present = is_present;
		is_present = other.is_present;
		if (was_present) {
			if (is_present)
				storage.value = other.storage.value;
			else
				storage.value.~T();
		} else {
			if (is_present)
				new (&storage.value) T(other.storage.value);
		}
	}
	explicit Option(T _value) : is_present(true) {
		new(&storage.value) T(_value);
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

	bool has() const { return is_present; }

	T& get() {
		assert(is_present);
		return storage.value;
	}
	const T& get() const {
		assert(is_present);
		return storage.value;
	}

	Option<const T&> as_ref() const {
		return is_present ? Option<const T&> { get() } : Option<const T&> {};
	}

	friend bool operator==(const Option<T>& a, const Option<T>& b) {
		return a.has() ? b.has() && a.get() == b.get() : !b.has();
	}
};

template <typename T>
class Option<T&> {
	T* ref;

public:
	Option() : ref(nullptr) {}
	explicit Option(T& value) : ref(&value) {}

	bool has() const { return ref != nullptr; }

	const T& get() const {
		assert(ref != nullptr);
		return *ref;
	}

	Option<typename std::remove_const<T>::type> copy_inner() const {
		if (has()) return Option<typename std::remove_const<T>::type> { *ref }; else return {};
	}
};

template <typename Out>
struct OptionMap {
	template <typename In, typename /*In => Out*/ Cb>
	Option<Out> operator()(const Option<In>& in, Cb cb) const {
		return in.has() ? Option<Out>{cb(in.get())} : Option<Out>{};
	}
};

template <typename Out>
OptionMap<Out> map() { return {}; }

template <typename T>
Option<ref<T>> un_ref(const Option<const ref<T>&> in) {
	return in.has() ? Option { in.get() } : Option<ref<T>>{};
}
template <typename T>
Option<ref<T>> to_ref(const Option<T&> in) {
	return in.has() ? Option { &in.get() } : Option<ref<T>>{};
}

template <typename Out>
struct MapOp {
	template <typename In, typename /*Option<In> => Option<Out>*/ Cb>
	Option<Out> operator()(const Option<In>& in, Cb cb) const {
		return in.has() ? Option { cb(in.get()) } : Option<Out>{};
	}
};
template <typename Out>
MapOp<Out> map_op() { return {}; };

template <typename T, typename /*() => Option<T>*/ Cb>
Option<T> or_option(const Option<T>& op, Cb cb) {
	return op.has() ? op : cb();
}

template <typename T>
class Late {
	Option<T> inner;

public:
	void init(T value) {
		assert(!inner.has()); // Only set once
		inner = value;
	}

	operator const T&() { return inner.get(); }
	const T& explicit_borrow() const { return inner.get(); } // TODO: shouldn't be necessary?
};
