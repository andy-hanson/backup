#include "./check_lifetime.h"

__attribute__((noreturn))
Lifetime common_lifetime(const Slice<Lifetime>& cases __attribute__((unused)), const Lifetime& elze __attribute__((unused))) {
	todo();
}

void check_return_lifetime(const Lifetime& declared, const Lifetime& actual) {
	if (declared.is_pointer()) {
		if (!actual.is_pointer()) {
			todo(); // In a return type, this is a no-no. `f Int *p(p Point)` *must* return a pointer into `p`.
		}

		// They should match, else we should warn user that they're declaring borrows they don't need.
		if (!declared.exactly_same_as(actual)) {
			todo();
		}
	} else {
		if (actual.is_pointer()) {
			// If actual.borrows_local(), then the error message should indicate that this is just impossible to do.
			// If actual borrows from parameters, then error message should tell user to mark return type with those parameters.
			todo(); // Diagnostic: return type must declare what it borrows from.
		}
		// Else both are 'new', nothing more to say.
	}
}
