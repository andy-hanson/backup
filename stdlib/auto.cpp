#include <iostream>
#include <limits>
using Bool = bool;

using Char = char;

using Int = int;

using Nat = unsigned int;

using Void = void;

struct String {
	const char* _begin;
	const char* _end;
	
	String(const char* begin, const char* end) : _begin(begin), _end(end) {}
	
	template <Nat N>
	// Note: the char array will include a '\0', but we don't want that included in the slice.
	constexpr String(char const (&c)[N]) : String(c, c + N - 1) { static_assert(N > 0); }
};

Bool empty(const String& s) {
	return s._begin == s._end;
}

Nat foo__add_Nat_Nat_Nat(const Nat& a, const Nat& b) {
	Nat res = a + b;
	if (res < a) throw "todo";
	return res;
}

Nat foo____Nat_Nat_Nat(const Nat& a, const Nat& b) {
	if (a < b) throw "todo";
	return a - b;
}

Nat foo__times_Nat_Nat_Nat(const Nat& a, const Nat& b) {
	if (a == 0)
		return 0;
	
	// a * b = res, so b = res / a
	// Res is at most int_max, so b is at most int_max / a.
	// Example: For 3-bit numbers, int_max is 7. If a is 4, int_max / a = 1. b is at most 1, b = 2 would overflow to 8.
	if (b > std::numeric_limits<Nat>::max() / a)
		throw "todo";
	
	return a * b;
}

Nat foo__div_Nat_Nat_Nat(const Nat& a, const Nat& b) {
	if (b == 0) throw "todo";
	return a / b;
}

Nat foo_mod_Nat_Nat_Nat(const Nat& a, const Nat& b) {
	if (b == 0) throw "todo";
	return a % b;
}

Int foo__add_Int_Int_Int(const Int& a, const Int& b) {
	// a + b = res, so b = res - a
	bool is_unsafe = a > 0
		// Res is at most int_max, to b is at most int_max - a.
		// a > 0, so int_max - a won't overflow.
		? b > std::numeric_limits<int>::max() - a
		// Res is at least int_min, to b is at least int_min - a.
		// a <= 0, so int_min - a won't overflow.
		: b < std::numeric_limits<int>::min() - a;
	if (is_unsafe) throw "todo";
	return a + b;
}

Int foo____Int_Int_Int(const Int& a, const Int& b) {
	// Note: can't just safe_add(a, -b) because negating int_min is undefined.
	// a - b = res, so  b = a - res.
	bool is_safe = a < 0
		? b > a - std::numeric_limits<Int>::min()
		: b < a - std::numeric_limits<Int>::max();
	if (!is_safe) throw "todo";
	return a + b;
}

Int foo__times_Int_Int_Int(const Int& a, const Int& b) {
	if (a == 0) // Avoid running int_max / 0
		return 0;
	// Doing this test to avoid running `int_min / -1`, which is UB if `int_min = -int_max - 1` (which it usually is)
	if (a == 1)
		return b;
	bool is_unsafe = a < 0
		? b < 0
			? b < std::numeric_limits<Int>::max() / a
			: b > std::numeric_limits<Int>::min() / a
		: b < 0
			? b < std::numeric_limits<Int>::min() / a
			: b > std::numeric_limits<Int>::max() / a;
	if (is_unsafe) throw "todo";
	return a * b;
}

Int foo__div_Int_Int_Int(const Int& a, const Int& b) {
	if (b == 0)throw "todo";
	return a / b;
}

Int foo_mod_Int_Int_Int(const Int& a, const Int& b) {
	if (b == 0) throw "todo";
	return a % b;
}

Nat foo_literal_Nat_String(const String& s) {
	if (empty(s)) throw "todo";
	const char *begin = s._begin;
	while (*begin == '0') ++begin;
	if (begin == s._end) return 0;
	Nat power = 1;
	Nat result = 0;
	
	for (const char *r = s._end - 1; ; --r) {
		char c = *r;
		if (c < '0' || c > '9') throw "todo";
		Nat digit = Nat(c - '0');
		if (digit > std::numeric_limits<Nat>::max() / power) throw "todo";
		Nat dp = digit * power;
		if (std::numeric_limits<Nat>::max() - result < dp) throw "todo";
		result += dp;
		if (r == begin) return result;
		if (power > std::numeric_limits<Nat>::max() / 10) throw "todo";
		power *= 10;
	}
}

Int to__int(const Nat& n) {
	if (n > std::numeric_limits<Int>::max()) throw "todo";
	return Int(n);
}

Nat to__nat(const Int& i) {
	if (i < 0) throw "todo";
	return Nat(i);
}

Int foo_literal_Int_String(const String& s) {
	if (empty(s)) throw "todo";
	const char* begin = s._begin;
	bool negate = false;
	if (*begin == '+') {
		++begin;
	} else if (*begin == '-') {
		++begin;
		negate = true;
	}
	Int res = to__int(foo_literal_Nat_String({ begin, s._end }));
	return negate ? res * -1 : res;
}

Void log(const Int& i) {
	std::cout << i << std::endl;
}

Int main() {
	log(foo_literal_Int_String("1"));
	return foo_literal_Int_String("0");
}

