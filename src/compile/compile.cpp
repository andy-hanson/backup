#include <iostream>
#include "./compile.h"

#include "../util/io.h"
#include "check/check.h"
#include "parse/parser.h"
#include "../host/DocumentProvider.h"

namespace {
	Option<ref<const Path>> resolve_import(ref<const Path> from, const ImportAst& i, PathCache& paths) {
		if (!i.n_parents.has()) throw "todo: global resolution";
		return paths.resolve(from, RelPath { i.n_parents.get(), i.path });
	}

	void parse_everything(Vec<FileAst>& out, Vec<Diagnostic>& diagnostics, ref<const Path> first_path, Arena& ast_arena, DocumentProvider& document_provider, PathCache& path_cache) {
		MaxSizeVector<16, ref<const Path>> to_parse;
		to_parse.push(first_path);
		Set<ref<const Path>> enqued_set; // Set of paths that are either already parsed, or in to_parse.
		enqued_set.must_insert(first_path);

		do {
			ref<const Path> path = to_parse.pop_and_return();
			Option<ArenaString> document = document_provider.try_get_document(path, ast_arena, NZ_EXTENSION);
			if (!document.has()) throw "todo: no such file";

			try {
				FileAst& f = out.emplace(FileAst { path, document.get(), {}, {} });
				parse_file(f, path_cache, ast_arena);
			} catch (ParseDiagnostic diag) {
				diagnostics.push({ path, diag });
				return;
			}

			for (const ImportAst& i : out.back().imports) {
				Option<ref<const Path>> op_dependency_path = resolve_import(path, i, path_cache);
				if (!op_dependency_path.has()) throw "todo";
				ref<const Path> dependency_path = op_dependency_path.get();

				if (enqued_set.insert(dependency_path).was_added)
					to_parse.push(dependency_path);
			}
		} while (!to_parse.empty());
	}

	Arr<ref<const Module>> get_imports(const Arr<ImportAst>& imports, ref<const Path> cur_path, const Map<ref<const Path>, ref<const Module>>& compiled, PathCache& paths, Arena& arena) {
		return arena.map<ref<const Module>>()(imports, [&](const ImportAst& a) {
			ref<const Path> p = resolve_import(cur_path, a, paths).get(); // Should succeed because we already did this in parse_everything
			Option<const ref<const Module>&> m = compiled.get(p);
			if (!m.has()) throw "todo: circular dependency";
			return m.get();
		});
	}
}

const StringSlice NZ_EXTENSION = "nz";

// Note: if there are any diagnostics, 'out' should not be used for anything other than printing them.
void compile(CompiledProgram& out, DocumentProvider& document_provider, ref<const Path> first_path) {
	Vec<FileAst> parsed;
	Arena ast_arena;
	parse_everything(parsed, out.diagnostics, first_path, ast_arena, document_provider, out.paths);
	if (!out.diagnostics.empty()) return;

	// Go in reverse order -- if we ever see some dependency that's not compiled yet, it indicates a circular dependency.
	Map<ref<const Path>, ref<const Module>> compiled;
	while (!parsed.empty()) {
		const FileAst& ast = parsed.back();
		ref<Module> m = out.arena.put(Module { ast.path, get_imports(ast.imports, ast.path, compiled, out.paths, out.arena) });
		out.modules.push(m);
		check(m, ast, out.arena, out.diagnostics);
		compiled.must_insert(m->path, m);
		parsed.pop();
	}
}

