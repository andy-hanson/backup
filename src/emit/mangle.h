#pragma once

#include "../compile/model/model.h"
#include "../util/Writer.h"

struct mangle { const StringSlice& name; };
Writer& operator<<(Writer& out, mangle man);
StringBuilder& operator<<(StringBuilder& out, mangle man);
