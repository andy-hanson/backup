#pragma once

#include "./ptr.h"
#include "./Slice.h"

class Arena {
	friend class StringBuilder; // TODO
	void* alloc_begin;
	void* alloc_next;
	void* alloc_end;

public:
	Arena();
	Arena(const Arena& other) = delete;
	void operator=(const Arena& other) = delete;
	~Arena();

	void* allocate(uint n_bytes);

	template <typename T>
	ref<T> allocate_uninitialized() {
		return static_cast<T*>(allocate(sizeof(T)));
	}

	template <typename T>
	ref<T> put(T value) {
		ref<T> ptr = allocate_uninitialized<T>();
		*ptr.ptr() = value;
		return ptr;
	}
};
