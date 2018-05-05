#include "./test.h"

#include <iostream>
#include "../compile/compile.h"
#include "../emit/emit.h"
#include "../host/DocumentProvider.h"
#include "../util/ArenaString.h"
#include "../util/io.h"
#include "../clang.h"

namespace {
	//TODO:MOVE
	template <typename A, typename B>
	bool collection_equal(const A& a, const B& b) {
		if (a.size() != b.size()) return false;

		typename A::const_iterator a_it = a.begin();
		typename B::const_iterator b_it = b.begin();
		typename A::const_iterator a_end = a.end();
		typename B::const_iterator b_end = b.end();
		while (true) {
			if (*a_it != *b_it) return false;
			++a_it;
			++b_it;
			if (a_it == a_end) {
				assert(b_it == b_end);
				return true;
			}
		}
	};

	Grow<char> diagnostics_baseline(const List<Diagnostic>& diags, DocumentProvider& document_provider) {
		Grow<char> out;
		Writer w { out };
		Arena temp;
		for (const Diagnostic& d : diags) {
			StringSlice document = document_provider.try_get_document(d.path, NZ_EXTENSION, temp).get();
			d.write(w, document, LineAndColumnGetter::for_text(document, temp));
			w << Writer::nl;
		}
		return out;
	}

	void no_baseline(const FileLocator& loc, TestMode mode) {
		if (file_exists(loc)) {
			switch (mode) {
				case TestMode::Test:
					throw "todo";
				case TestMode::Accept:
					delete_file(loc);
					break;
			}
		}
	}

	struct TestFailure {};

	std::ostream& operator<<(std::ostream& out, const StringSlice& slice) {
		for (char c : slice)
			out << c;
		return out;
	}

	std::ostream& operator<<(std::ostream& out, const Path& path) {
		const Option<Path>& parent = path.parent();
		if (parent.has()) {
			out << parent.get();
			out << '/';
		}
		return out << path.base_name();
	}

	std::ostream& operator<<(std::ostream& out, const FileLocator& loc) {
		return out << loc.root << '/' << loc.path << '.' << loc.extension;
	}

	std::ostream& operator<<(std::ostream& out, const Grow<char>& g) {
		for (char c : g)
			out << c;
		return out;
	}

	// Returns true on success. Always succeeds with TestMode::Accept
	void baseline(const FileLocator& loc, const Grow<char>& actual, TestMode mode) {
		Arena expected_arena;
		Option<StringSlice> expected = try_read_file(loc, expected_arena, /*null_terminated*/ false);
		if (expected.has()) {
			if (!collection_equal(actual, expected.get())) {
				switch (mode) {
					case TestMode::Test:
						std::cerr << "Unexpected result for baseline " << loc << ". Actual: \n" << actual << std::endl;
						throw TestFailure {};
					case TestMode::Accept:
						write_file(loc, actual);
				}
			}
		} else {
			switch (mode) {
				case TestMode::Test:
					std::cerr << "There is now a baseline for " << loc << ": " << std::endl
						<< ">>>" << std::endl << actual << std::endl << "<<<" << std::endl;
					throw TestFailure {};
				case TestMode::Accept:
					write_file(loc, actual);
			}
		}
	}

	void do_test(StringSlice root, TestMode mode) {
		unique_ptr<DocumentProvider> document_provider = file_system_document_provider(root);

		CompiledProgram out;
		Path main_path = out.paths.from_part_slice("main");
		compile(out, *document_provider, main_path);

		FileLocator diags_path = { root, out.paths.from_part_slice("diagnostics-baseline"), "txt" };
		FileLocator cpp_path = { root, main_path, "cpp" };
		FileLocator exe_path { root, main_path, "exe" };

		if (out.diagnostics.empty()) {
			no_baseline(diags_path, mode);
			baseline({ root, main_path, "cpp" }, emit(out.modules), mode);
			compile_cpp_file(cpp_path, exe_path);
			int res = execute_file(exe_path);
			if (res != 0) {
				std::cerr << exe_path << " exit code: " << res << std::endl;
				throw TestFailure {};
			}
		} else {
			baseline(diags_path, diagnostics_baseline(out.diagnostics, *document_provider), mode);
			no_baseline(cpp_path, mode);
			no_baseline(exe_path, mode);
		}
	}
}

void test(const StringSlice& test_dir, const StringSlice& test_name, TestMode mode) {
	Arena a;
	StringBuilder s { a, test_dir.size() + 1 + test_name.size() };
	s << test_dir << '/' << test_name;
	try {
		do_test(s.finish(), mode);
	} catch (TestFailure) {
		std::cerr << "There was a test failure." << std::endl;
	}
}
