#include "./test.h"

#include <iostream> // std::cerr, std::ostream
#include "../util/MaxSizeMap.h"
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

	std::ostream& operator<<(std::ostream& out, const TestFailure& failure) {
		out << failure.loc << ' ';
		switch (failure.kind) {
			case TestFailure::Kind::UnexpectedBaseline:
				out << "Baseline result changed.";
				break;
			case TestFailure::Kind::CppCompilationFailed:
				out << ".cpp compilation failed.";
				break;
		}
		return out;
	}

	using DirectoriesTodo = MaxSizeVector<64, Path>;
	//using Files = MaxSizeSet<64, Path, Path::hash>;

	struct TestDirectoryIteratee : DirectoryIteratee {
		PathCache& paths;
		Option<Path> directory_path;
		DirectoriesTodo to_test;
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

	void do_test_recur(const StringSlice& dir, TestMode mode, PathCache& paths, List<TestFailure>::Builder& failures, Arena& arena) {
		TestDirectoryIteratee iteratee { paths, {} };
		list_directory(dir, iteratee);
		if (iteratee.any_files) {
			if (!iteratee.main_nz) throw "todo";
			test_single(dir, mode, paths, failures, arena);
		} else {
			for (const Path& directory_path : iteratee.to_test) {
				MaxSizeString<256> nested;
				MutableStringSlice m = nested.slice();
				directory_path.write(m, dir, {});
				do_test_recur(StringSlice(nested.slice()).slice(0, to_unsigned(m.begin - nested.slice().begin)), mode, paths, failures, arena);
			}
		}
	}

	int do_test(const StringSlice& root_dir, TestMode mode) {
		PathCache paths;
		List<TestFailure>::Builder failures_builder;
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
