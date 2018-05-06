#pragma once

#include "../util/store/Arena.h"
#include "../util/store/ArenaString.h"
#include "../util/Writer.h"

void emit_comment(Writer& writer, const Option<ArenaString>& comment);
