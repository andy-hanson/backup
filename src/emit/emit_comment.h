#pragma once

#include "../util/Alloc.h"
#include "../util/Writer.h"

void emit_comment(Writer& writer, const Option<ArenaString>& comment);
