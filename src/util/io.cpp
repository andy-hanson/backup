#include "./io.h"

// TODO: c++17 #include <filesystem> and rewrite everything...
#include <dirent.h> // readdir_r
#include <fstream> // std::ifstream, std::ofstream, std::remove
#include <cstring> // strlen
#include "./ArenaString.h"

namespace {
	using PathString = MaxSizeString<128>;

	bool is_directory(unsigned char d_type) {
		switch (d_type) {
			case DT_REG: // file
				return false;
			case DT_DIR: // directory
				return true;
			default:
				unreachable();
		}
	}
}

DirectoryIteratee::~DirectoryIteratee() {}

Option<StringSlice> try_read_file(const FileLocator& loc, Arena& out, bool null_terminated) {
	PathString temp;
	const char* c_path = loc.get_cstring(temp);
	std::ifstream i(c_path);
	if (!i) return {};

	i.seekg(0, std::ios::end);
	long signed_size = i.tellg();
	uint size = to_unsigned(signed_size);
	ArenaString res = allocate_slice(out, size + null_terminated);
	i.seekg(0);
	i.read(res.begin(), signed_size);
	assert(i.gcount() == signed_size);
	if (null_terminated) res[size] = '\0';
	return Option { res.slice() };
}

void list_directory(const StringSlice& loc, DirectoryIteratee& iteratee) {
	PathString dir_path;
	dir_path.slice() << loc << '\0';
	Ref<DIR> dir = opendir(dir_path.slice().begin);

	while (true) {
		dirent* ent = readdir(dir.ptr());
		if (ent == nullptr) {
			assert(errno == 0);
			break;
		}

		const StringSlice& name { ent->d_name, ent->d_name + strlen(ent->d_name) };
		if (is_directory(ent->d_type)) {
			if (name != ".." && name != ".")
				iteratee.on_directory(name);
		} else
			iteratee.on_file(name);
	}

	closedir(dir.ptr());
}

void write_file(const FileLocator& loc, const BlockedList<char>& contents) {
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
