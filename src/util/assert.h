#pragma once

inline constexpr void assert(bool b) {
	if (!b) throw "todo";
}

__attribute__((__noreturn__))
inline void unreachable() {
	throw "todo";
}

__attribute__((__noreturn__))
inline void todo() {
	throw "todo";
}

