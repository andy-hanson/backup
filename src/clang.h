#pragma once

#include "./util/io.h"

int execute_file(const FileLocator& file_path);
void compile_cpp_file(const FileLocator& cpp_file_name, const FileLocator& exe_file_name);
