#pragma once

#include "./assert.h"
#include "./Ref.h"

template <typename T>
class Option {
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
	Option(const Option<T>& other) : is_present(other.is_present) {
		//TODO: new(&storage.value) T(other.storage.value);
		storage.value = other.storage.value;
	}
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
				//TODO: new (&storage.value) T(other.storage.value);
				storage.value = other.storage.value;
		}
	}
	explicit Option(T _value) : is_present(true) {
		//TODO: new(&storage.value) T(_value);
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

	T& get() {
		assert(ref != nullptr);
		return *ref;
	}
	const T& get() const {
		assert(ref != nullptr);
		return *ref;
	}
};

template <typename T>
Option<Ref<T>> copy_inner(const Option<const Ref<T>&> o) {
	if (o.has()) return Option<Ref<T>> { o.get() }; else return {};
}

template <typename T>
Option<Ref<T>> un_ref(const Option<const Ref<T>&> in) {
	return in.has() ? Option { in.get() } : Option<Ref<T>>{};
}

template <typename Out>
struct MapOption {
	template <typename In, typename /*Option<In> => Option<Out>*/ Cb>
	Option<Out> operator()(const Option<In>& in, Cb cb) const {
		return in.has() ? Option { cb(in.get()) } : Option<Out>{};
	}
};
template <typename Out>
MapOption<Out> map_option() { return {}; };

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
