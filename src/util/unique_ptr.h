#pragma once

template <typename T>
class unique_ptr {
	T* _ptr; // non-null

public:
	unique_ptr(const unique_ptr& other) = delete;

	// Must pass in a 'new' instance here
	inline unique_ptr(T* ptr) : _ptr(ptr) {
		assert(ptr != nullptr);
	}

	~unique_ptr() {
		assert(_ptr != nullptr);
		delete _ptr;
		_ptr = nullptr;
	}

	inline T& operator*() { return *_ptr; }
	inline const T& operator*() const { return *_ptr; }
	inline T* operator->() { return _ptr; }
	inline const T* operator->() const { return _ptr; }
};
