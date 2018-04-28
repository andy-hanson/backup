#include "./DocumentProvider.h"

#include <cassert>
#include <fstream>

namespace {
	struct FileDocumentProvider final : public DocumentProvider {
		const std::string root;
		FileDocumentProvider(std::string _root) : root(_root) {}
		Option<ArenaString> try_get_document(const Path& path, Arena& out, const StringSlice& extension) override {
			const char* c_path = path.to_cstring(root, out, extension).slice().cstr();
			std::ifstream i(c_path);
			if (!i) return {};

			i.seekg(0, std::ios::end);
			long signed_size = i.tellg();
			size_t size = to_unsigned(signed_size);
			ArenaString res = out.allocate_slice(size + 1);
			i.seekg(0);
			i.read(res.begin(), signed_size);
			assert(i.gcount() == signed_size);
			res[size] = '\0';
			return Option{res};
		}
	};
}

DocumentProvider::~DocumentProvider() {}

std::unique_ptr<DocumentProvider> file_system_document_provider(const std::string& root) {
	return std::make_unique<FileDocumentProvider>(root);
}
