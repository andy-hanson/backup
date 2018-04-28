#pragma once

#include <cstddef>

using uint = unsigned int;
using ushort = unsigned short;
using ulong = unsigned long;

size_t to_unsigned(std::ptrdiff_t s);
uint to_uint(size_t u);
ushort to_ushort(size_t u);
