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
	Option() : is_present{false} {}
	Option(const Option<T>& other) : is_present{other.is_present} {
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
	explicit Option(T _value) : is_present{true} {
		//TODO: new(&storage.value) T(_value);
		storage.value = _value;
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
};
template <typename T>
inline bool operator==(const Option<T>& a, const Option<T>& b) {
	return a.has() ? b.has() && a.get() == b.get() : !b.has();
}
template <typename T>
inline bool operator!=(const Option<T>& a, const Option<T>& b) {
	return !(a == b);
}

template <typename T>
class Option<T&> {
	T* ref;

public:
	inline Option() : ref{nullptr} {}
	inline explicit Option(T& value) : ref{&value} {}

	inline bool has() const { return ref != nullptr; }

	inline T& get() {
		assert(ref != nullptr);
		return *ref;
	}
	inline const T& get() const {
		assert(ref != nullptr);
		return *ref;
	}
};

template <typename T>
class Option<Ref<T>> {
	T* ref;

public:
	inline Option() : ref{nullptr} {}
	inline explicit Option(Ref<T> value) : ref{value.ptr()} {}

	inline void operator=(Ref<T> value) {
		ref = value.ptr();
	}

	inline bool has() const { return ref != nullptr; }

	inline Ref<T> get() {
		return Ref<T> { ref };
	}
	inline Ref<const T> get() const {
		return Ref<const T> { ref };
	}
};

template <typename T>
Option<Ref<T>> copy_inner(const Option<const Ref<T>&> o) {
	if (o.has()) return Option<Ref<T>> { o.get() }; else return {};
}

template <typename Out>
struct map_option {
	template <typename In, typename /*Option<In> => Option<Out>*/ Cb>
	Option<Out> operator()(const Option<In>& in, Cb cb) const {
		return in.has() ? cb(in.get()) : Option<Out>{};
	}
};

template <typename T, typename /*() => Option<T>*/ Cb>
Option<T> or_option(const Option<T>& op, Cb cb) {
	return op.has() ? op : cb();
}

template <typename T>
class Late {
	Option<T> inner;

public:
	inline Late() {}
	inline Late(T value) : inner{value} {}

	inline void init(T value) {
		assert(!inner.has()); // Only set once
		inner = value;
	}

	inline const T& get() const { return inner.get(); }
};
