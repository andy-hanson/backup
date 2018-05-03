#include "./compile.h"

#include "../util/Grow.h"
#include "../util/MaxSizeMap.h"
#include "../host/DocumentProvider.h"
#include "./check/check.h"
#include "./parse/parser.h"

namespace {
	Option<Path> resolve_import(Path from, const ImportAst& i, PathCache& paths) {
		if (!i.n_parents.has()) throw "todo: global resolution";
		return paths.resolve(from, RelPath { i.n_parents.get(), i.path });
	}

	void parse_everything(Grow<FileAst>& out, Grow<Diagnostic>& diagnostics, Path first_path, Arena& ast_arena, DocumentProvider& document_provider, PathCache& path_cache) {
		MaxSizeVector<16, Path> to_parse;
		to_parse.push(first_path);
		MaxSizeSet<16, Path, Path::hash> enqued_set; // Set of paths that are either already parsed, or in to_parse.
		enqued_set.must_insert(first_path);

		do {
			Path path = to_parse.pop_and_return();
			Option<ArenaString> document = document_provider.try_get_document(path, ast_arena, NZ_EXTENSION);
			if (!document.has()) throw "todo: no such file";

			try {
				FileAst& f = out.emplace(path, document.get());
				parse_file(f, path_cache, ast_arena);
			} catch (ParseDiagnostic diag) {
				diagnostics.push({ path, diag });
				return;
			}

			for (const ImportAst& i : out.back().imports) {
				Option<Path> op_dependency_path = resolve_import(path, i, path_cache);
				if (!op_dependency_path.has()) throw "todo";
				Path dependency_path = op_dependency_path.get();

				if (enqued_set.insert(dependency_path).was_added)
					to_parse.push(dependency_path);
			}
		} while (!to_parse.empty());
	}

	using Compiled = MaxSizeMap<16, Path, ref<const Module>, Path::hash>;

	Option<Arr<ref<const Module>>> get_imports(
		const Arr<ImportAst>& imports, Path cur_path, const Compiled& compiled, PathCache& paths, Arena& arena, Grow<Diagnostic>& diagnostics
	) {
		return arena.map_or_fail<ref<const Module>>()(imports, [&](const ImportAst& ast) {
			Path p = resolve_import(cur_path, ast, paths).get(); // Should succeed because we already did this in parse_everything
			Option<const ref<const Module>&> m = compiled.get(p);
			if (!m.has()) {
				diagnostics.push({ cur_path, ast.range, Diag::Kind::CircularImport });
				return Option<ref<const Module>>{};
			}
			return Option { m.get() };
		});
	}
}

const StringSlice NZ_EXTENSION = "nz";

// Note: if there are any diagnostics, 'out' should not be used for anything other than printing them.
void compile(CompiledProgram& out, DocumentProvider& document_provider, Path first_path) {
	Grow<FileAst> parsed;
	Arena ast_arena;
	parse_everything(parsed, out.diagnostics, first_path, ast_arena, document_provider, out.paths);
	if (!out.diagnostics.empty()) return;

	// Go in reverse order -- if we ever see some dependency that's not compiled yet, it indicates a circular dependency.
	Compiled compiled;
	Option<Arr<Module>> modules = out.arena.map_or_fail_reverse<Module>()(parsed, [&](const FileAst& ast, ref<Module> m) {
		Option<Arr<ref<const Module>>> imports = get_imports(ast.imports, ast.path, compiled, out.paths, out.arena, out.diagnostics);
		new (m.ptr()) Module { ast.path, imports.get(), ast.comment.has() ? Option { out.arena.str(ast.comment.get()) } : Option<ArenaString> {} };
		if (!imports.has()) {
			assert(!out.diagnostics.empty());
			return false;
		}
		check(m, ast, out.arena, out.diagnostics);
		if (!out.diagnostics.empty())
			return false;
		compiled.must_insert(m->path, m);
		return true;
	});
	if (modules.has())
		out.modules = modules.get();
}
