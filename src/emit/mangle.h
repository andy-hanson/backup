#pragma once

#include "Writer.h"
#include "../model/model.h"

struct mangle { const Identifier& name; };
Writer& operator<<(Writer& out, mangle man);
Arena::StringBuilder& operator<<(Arena::StringBuilder& out, mangle man);
