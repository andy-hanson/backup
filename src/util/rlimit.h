#pragma once

void set_limits();
void unset_limits();

template <typename /*() => void*/ Cb>
void without_limits(Cb cb) {
	unset_limits();
	cb();
	set_limits();
}
