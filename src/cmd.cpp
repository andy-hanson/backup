#include "./cmd.h"

#include <iostream>
#include "./compile/compile.h"
#include "./emit/emit.h"
#include "./util/io.h"
#include "./clang.h"

void run(const std::string& file_name) {
	std::system((file_name + ".exe").c_str());
}

void compile_and_run(const std::string& file_name) {
	compile_nz_file(file_name);
	run(file_name);
}

std::string compile_to_string(const std::string& file_name) {
	CompiledProgram program;
	std::unique_ptr<DocumentProvider> dp = file_system_document_provider("/");
	compile(program, *dp, 	program.paths.from_string(file_name));
	return emit(program.modules);
}

void compile_nz_file(const std::string& file_name) {
	std::string emitted = compile_to_string(file_name);
	std::string cpp = file_name + ".cpp";
	write_file(cpp, emitted);
	compile_cpp_file(cpp, file_name + ".exe");
}
