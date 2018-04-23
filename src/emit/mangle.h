#pragma once

#include "../compile/model/model.h"
#include "../util/Writer.h"

struct mangle { const Identifier& name; };
Writer& operator<<(Writer& out, mangle man);
Arena::StringBuilder& operator<<(Arena::StringBuilder& out, mangle man);
