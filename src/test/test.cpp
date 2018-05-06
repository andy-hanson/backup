#include "./test.h"

#include <iostream> // std::cerr, std::ostream
#include "../util/store/ListBuilder.h"
#include "../compile/compile.h"
#include "./test_single.h"

namespace {
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

	const char* baseline_name(TestFailure::Kind kind) {
		switch (kind) {
			case TestFailure::Kind::BaselineAdded:
				return "Baseline added.";
			case TestFailure::Kind::BaselineChanged:
				return "Baseline changed";
			case TestFailure::Kind::BaselineRemoved:
				return "Baseline removed";
			case TestFailure::Kind::CppCompilationFailed:
				return ".cpp compilation failed.";
		}
	}

	std::ostream& operator<<(std::ostream& out, const TestFailure& failure) {
		return out << failure.loc << ' ' << baseline_name(failure.kind);
	}

	struct TestDirectoryIteratee : DirectoryIteratee {
		PathCache& paths;
		Option<Path> directory_path;
		MaxSizeVector<64, Path> to_test;
		bool any_files;
		bool main_nz;
		TestDirectoryIteratee(PathCache& _paths, Option<Path> _directory_path) : paths(_paths), directory_path(_directory_path), to_test{}, any_files{false}, main_nz{false} {}

		void on_file(const StringSlice& name) override {
			any_files = true;
			main_nz = main_nz || name == "main.nz";
		}
		void on_directory(const StringSlice& name) override {
			to_test.push(paths.resolve(directory_path, name));
		}
	};

	void do_test_recur(const StringSlice& dir, TestMode mode, PathCache& paths, ListBuilder<TestFailure>& failures, Arena& arena) {
		TestDirectoryIteratee iteratee { paths, {} };
		list_directory(dir, iteratee);
		if (iteratee.any_files) {
			if (!iteratee.main_nz) todo(); // Non-test directory?
			test_single(dir, mode, paths, failures, arena);
		} else {
			for (const Path& directory_path : iteratee.to_test) {
				MaxSizeStringStorage<256> nested;
				MaxSizeStringWriter slice = nested.writer();
				directory_path.write(slice, dir, {});
				do_test_recur(nested.finish(slice), mode, paths, failures, arena);
			}
		}
	}

	int do_test(const StringSlice& root_dir, TestMode mode) {
		PathCache paths;
		ListBuilder<TestFailure> failures_builder;
		Arena arena;
		do_test_recur(root_dir, mode, paths, failures_builder, arena);

		List<TestFailure> failures = failures_builder.finish();
		for (const TestFailure& failure : failures)
			std::cerr << failure << std::endl;

		return to_signed(failures.size());
	}
}

int test(const StringSlice& test_dir,  TestMode mode) {
	return do_test(test_dir, mode);
}
