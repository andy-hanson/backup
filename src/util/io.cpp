#include "./io.h"

#include <fstream>

using PathString = MaxSizeString<128>;

Option<ArenaString> try_read_file(const FileLocator& loc, Arena& out, bool null_terminated) {
	PathString temp;
	const char* c_path = loc.get_cstring(temp);
	std::ifstream i(c_path);
	if (!i) return {};

	i.seekg(0, std::ios::end);
	long signed_size = i.tellg();
	size_t size = to_unsigned(signed_size);
	ArenaString res = out.allocate_slice(size + null_terminated);
	i.seekg(0);
	i.read(res.begin(), signed_size);
	assert(i.gcount() == signed_size);
	if (null_terminated) res[size] = '\0';
	return Option{res};
}

void write_file(const FileLocator& loc, const Grow<char>& contents) {
	PathString temp;
	const char* c_path = loc.get_cstring(temp);
	std::ofstream out(c_path);
	for (char c : contents)
		out << c;
	out.close();
}

void delete_file(const FileLocator& loc) {
	PathString temp;
	std::remove(loc.get_cstring(temp));
}

bool file_exists(const FileLocator& loc) {
	//todo:c++17 std::filesystem::exists would be nice
	PathString temp;
	return bool(std::ifstream(loc.get_cstring(temp)));
}
