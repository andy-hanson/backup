#pragma once

enum class Effect { Pure, Get, Set, Io };
StringSlice effect_name(Effect e);
