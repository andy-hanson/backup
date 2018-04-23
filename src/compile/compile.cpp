#include <iostream>
#include "./compile.h"

#include "../util/io.h"
#include "check/check.h"
#include "parse/parser.h"

namespace {
	ref<Module> parse_and_check_file(StringSlice file_path, Identifier module_name, StringSlice file_content, Arena& arena) {
		Vec<DeclarationAst> declarations;
		ref<Module> m = arena.put(Module { arena.str(file_path), module_name });
		try {
			check(m, file_content, parse_file(file_content, arena), arena);
			return m;
		} catch (ParseDiagnostic p) {
			m->diagnostics.push(p);
			return m;
		}
	}
}

void compile(CompiledProgram& out, const std::string& file_name) {
	std::string nz_file_name = file_name + ".nz";
	std::string nz_file_content = read_file(file_name + ".nz");
	Arena arena;
	Identifier module_name = Identifier{arena.str("foo")}; //TODO
	out.modules.push(parse_and_check_file(nz_file_name, module_name, nz_file_content, arena));
	for (const Diagnostic& d : out.modules[0]->diagnostics) {
		Writer w;
		d.write(w, nz_file_content);
		std::cout << "There was an error:\n" << w.finish() << std::endl;
	}
	if (!out.modules[0]->diagnostics.empty()) throw "todo";
}
