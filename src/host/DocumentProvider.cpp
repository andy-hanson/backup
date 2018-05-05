#include "./DocumentProvider.h"

#include "../util/io.h"

namespace {
	struct FileDocumentProvider final : public DocumentProvider {
		const StringSlice root;
		FileDocumentProvider(StringSlice _root) : root(_root) {}
		Option<StringSlice> try_get_document(const Path& path, const StringSlice& extension, Arena& out) override {
			return try_read_file({ root, path, extension }, out, /*null_terminated*/ true);
		}
	};
}

DocumentProvider::~DocumentProvider() {}

unique_ptr<DocumentProvider> file_system_document_provider(StringSlice root) {
	return unique_ptr<DocumentProvider> { new FileDocumentProvider(root) };
}
