#include "./test.h"

#include <iostream>
#include "../compile/compile.h"
#include "../emit/emit.h"
#include "../host/DocumentProvider.h"
#include "../util/io.h"
#include "../clang.h"

namespace {
	std::string diagnostics_baseline(const Grow<Diagnostic>& diags, DocumentProvider& document_provider) {
		Writer w;
		Arena temp;
		for (const Diagnostic& d : diags) {
			StringSlice document = document_provider.try_get_document(d.path, temp, NZ_EXTENSION).get();
			d.write(w, document, LineAndColumnGetter::for_text(document, temp));
			w << Writer::nl;
		}
		return w.finish();
	}

	void no_baseline(const std::string& file_path, TestMode mode) {
		if (file_exists(file_path)) {
			switch (mode) {
				case TestMode::Test:
					throw "todo";
				case TestMode::Accept:
					delete_file(file_path);
					break;
			}
		}
	}

	struct TestFailure {};

	// Returns true on success. Always succeeds with TestMode::Accept
	void baseline(const std::string& file_path, const std::string& actual, TestMode mode) {
		Option<std::string> expected = try_read_file(file_path);
		if (expected.has()) {
			if (actual != expected.get()) {
				switch (mode) {
					case TestMode::Test:
						std::cerr << "Unexpected result for baseline " << file_path << ". Actual: \n" << actual << std::endl;
						throw TestFailure {};
					case TestMode::Accept:
						write_file(file_path, actual);
				}
			}
		} else {
			switch (mode) {
				case TestMode::Test:
					std::cerr << "There is now a baseline for " << file_path << ": " << std::endl
						<< ">>>" << std::endl << actual << std::endl << "<<<" << std::endl;
					throw TestFailure {};
				case TestMode::Accept:
					write_file(file_path, actual);
			}
		}
	}

	void do_test(StringSlice root, TestMode mode) {
		unique_ptr<DocumentProvider> document_provider = file_system_document_provider(root);

		CompiledProgram out;
		Path first_path = out.paths.from_part_slice("main");
		compile(out, *document_provider, first_path);

		std::string root_s { root.begin(), root.end() };
		std::string diags_path = root_s + "/diagnostics-baseline.txt";
		std::string cpp_path = root_s + "/main.cpp";
		std::string exe_path = root_s + "/main.exe";

		if (out.diagnostics.empty()) {
			no_baseline(diags_path, mode);
			baseline(cpp_path, emit(out.modules), mode);
			compile_cpp_file(cpp_path, exe_path);
			int res = execute_file(exe_path.c_str());
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
	Arena::StringBuilder s = a.string_builder(test_dir.size() + 1 + test_name.size());
	s << test_dir << '/' << test_name;
	try {
		do_test(s.finish(), mode);
	} catch (TestFailure) {
		std::cerr << "There was a test failure." << std::endl;
	}
}
