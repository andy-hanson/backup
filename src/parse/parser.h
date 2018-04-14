#pragma once

#include "../model/model.h"

ref<Module> parse_file(const StringSlice& file_path, const Identifier& module_name, const StringSlice& file_content, Arena& arena);
