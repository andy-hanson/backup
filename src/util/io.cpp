#include <fstream>
#include <cassert>
#include "./io.h"
#include "./int.h"
#include "../host/Path.h"

Option<std::string> try_read_file(const std::string& file_name) {
	std::ifstream i(file_name);
	return i ? Option{std::string(std::istreambuf_iterator<char>(i), std::istreambuf_iterator<char>())} : Option<std::string>{};
}

void write_file(const std::string& file_name, const std::string& contents) {
	std::ofstream out(file_name);
	out << contents;
	out.close();
}

void delete_file(const std::string& file_name) {
	std::remove(file_name.c_str());
}

bool file_exists(const std::string& file_name) {
	//todo: std::filesystem::exists would be nice
	return bool(std::ifstream(file_name));
}
