#pragma once

#include <string>

std::string compile_to_string(const std::string& file_name);
void compile_nz_file(const std::string& file_name);
void run(const std::string& file_name);
void compile_and_run(const std::string& file_name);
