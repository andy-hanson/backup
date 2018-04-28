#pragma once

#include <memory> // unique_ptr
#include "../util/Alloc.h"
#include "./Path.h"

/*abstract*/ class DocumentProvider {
public:
	// Should be 0 terminated, meaning s[s.size() - 1] == '\0'
	virtual Option<ArenaString> try_get_document(const Path& path, Arena& out, const StringSlice& extension) = 0;
	// https://stackoverflow.com/a/29217604
	// If we make this '= 0' there is a compiler warning.
	virtual ~DocumentProvider();
};

std::unique_ptr<DocumentProvider> file_system_document_provider(const std::string& root);
