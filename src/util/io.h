#pragma once

#include <string>

std::string read_file(const std::string& file_name);
void write_file(const std::string& file_name, const std::string& contents);
void delete_file(const std::string& file_name);
bool file_exists(const std::string& file_name);
