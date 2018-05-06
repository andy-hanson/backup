#include "./test_single.h"

#include "../compile/compile.h"
#include "../emit/emit.h"
#include "../host/DocumentProvider.h"
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

	Writer::Output diagnostics_baseline(const List<Diagnostic>& diags, DocumentProvider& document_provider, Arena& arena) {
		Writer out { arena };
		Arena temp;
		for (const Diagnostic& d : diags) {
			StringSlice document = document_provider.try_get_document(d.path, NZ_EXTENSION, temp).get();
			d.write(out, document, LineAndColumnGetter::for_text(document, temp));
			out << Writer::nl;
		}
		return out.finish();
	}

	void no_baseline(const FileLocator& loc, TestMode mode, ListBuilder<TestFailure>& failures, Arena& failures_arena) {
		if (file_exists(loc)) {
			switch (mode) {
				case TestMode::Test:
					failures.add({ TestFailure::Kind::BaselineRemoved, loc }, failures_arena);
					todo();
				case TestMode::Accept:
					delete_file(loc);
					break;
			}
		}
	}

	// Returns true on success. Always succeeds with TestMode::Accept
	void baseline(const FileLocator& loc, const StringSlice& error_extension, const Writer::Output& actual, TestMode mode, ListBuilder<TestFailure>& failures, Arena& failures_arena) {
		bool should_write_new = false;
		bool should_delete_new = false;
		bool should_overwrite = false;
		Arena expected_arena; //TODO:PERF
		Option<StringSlice> expected = try_read_file(loc, expected_arena, /*null_terminated*/ false);
		if (expected.has()) {
			if (collection_equal(actual, expected.get()))
				should_delete_new = true;
			else {
				switch (mode) {
					case TestMode::Test:
						should_write_new = true;
						break;
					case TestMode::Accept:
						should_overwrite = true;
						should_delete_new = true;
						break;
				}
			}
		} else {
			switch (mode) {
				case TestMode::Test:
					should_write_new = true;
					break;
				case TestMode::Accept:
					should_overwrite = true;
					should_delete_new = true;
					break;
			}
		}

		if (should_write_new) {
			write_file(loc.with_extension(error_extension), actual);
			failures.add({ expected.has() ? TestFailure::Kind::BaselineChanged : TestFailure::Kind::BaselineAdded, loc }, failures_arena);
		} else if (should_delete_new) {
			delete_file(loc.with_extension(error_extension));
			if (should_overwrite)
				write_file(loc, actual);
		}
	}
}

void test_single(const StringSlice& root, TestMode mode, PathCache& paths, ListBuilder<TestFailure>& failures, Arena& failures_arena) {
	unique_ptr<DocumentProvider> document_provider = file_system_document_provider(root);

	CompiledProgram out;
	Path out_main_path = out.paths.from_part_slice("main");
	compile(out, *document_provider, out_main_path);

	Path main_path = paths.from_part_slice("main");

	FileLocator diags_path = { root, paths.from_part_slice("diagnostics-baseline"), "txt" };
	FileLocator cpp_path = { root, main_path, "cpp" };
	FileLocator exe_path { root, main_path, "exe" };

	if (out.diagnostics.is_empty()) {
		Arena temp; //TODO:PERF
		no_baseline(diags_path, mode, failures, failures_arena);
		baseline({ root, main_path, "cpp" }, "cpp.new", emit(out.modules, temp), mode, failures, failures_arena);
		compile_cpp_file(cpp_path, exe_path); // No error if this produces different code... that's clang's problem
		int exit_code = execute_file(exe_path);
		if (exit_code != 0)
			failures.add({ TestFailure::Kind::CppCompilationFailed, cpp_path }, failures_arena);
	} else {
		Arena temp; //TODO:PERF
		baseline(diags_path, "txt.new", diagnostics_baseline(out.diagnostics, *document_provider, temp), mode, failures, failures_arena);
		no_baseline(cpp_path, mode, failures, failures_arena);
		no_baseline(exe_path, mode, failures, failures_arena);
	}
}
