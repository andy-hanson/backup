#include "MaxSizeStringStorage.h"

MaxSizeStringWriter& MaxSizeStringWriter::operator<<(const StringSlice& s) {
	for (char c : s)
		*this << c;
	return *this;
}
