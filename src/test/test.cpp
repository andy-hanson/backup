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
		return out << failure.loc.slice() << ' ' << baseline_name(failure.kind);
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

	// Returns the # tests run.
	uint do_test_recur(const StringSlice& dir, const TestFilter& filter, TestMode mode, PathCache& paths, ListBuilder<TestFailure>& failures, Arena& arena) {
		TestDirectoryIteratee iteratee { paths, {} };
		list_directory(dir, iteratee);
		if (iteratee.any_files) {
			if (!iteratee.main_nz) todo(); // Non-test directory?
			if (filter.should_test(dir)) {
				std::cout << "testing " << dir << std::endl;
				test_single(dir, mode, paths, failures, arena);
				return 1;
			}
			return 0;
		} else {
			uint n_tests_run = 0;
			for (const Path& directory_path : iteratee.to_test) {
				MaxSizeString<256> nested = MaxSizeString<256>::make([&](MaxSizeStringWriter& w) {
					directory_path.write(w, dir, {});
				});
				n_tests_run += do_test_recur(nested.slice(), filter, mode, paths, failures, arena);
			}
			return n_tests_run;
		}
	}
}

TestFilter::~TestFilter() {}

bool EveryTestFilter::should_test(const StringSlice& directory __attribute__((unused))) const { return true; }

int test(const StringSlice& test_dir, const TestFilter& filter, TestMode mode) {
	PathCache paths;
	ListBuilder<TestFailure> failures_builder;
	Arena arena;
	uint n_tests_run = do_test_recur(test_dir, filter, mode, paths, failures_builder, arena);
	std::cout << n_tests_run << " tests run" << std::endl;

	List<TestFailure> failures = failures_builder.finish();
	for (const TestFailure& failure : failures)
		std::cerr << failure << std::endl;
	if (!failures.is_empty())
		std::cerr << failures.size() << " failures" << std::endl;

	return to_signed(failures.size());
}
