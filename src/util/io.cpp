#include "./io.h"

#include <fstream>

std::string read_file(const std::string& file_name) {
	std::ifstream i(file_name);
	if (!i) throw "todo";
	return std::string(std::istreambuf_iterator<char>(i), std::istreambuf_iterator<char>());
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
