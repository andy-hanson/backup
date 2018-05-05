#include "./compile.h"

#include "../util/MaxSizeMap.h"
#include "./check/check.h"
#include "./parse/parser.h"

namespace {
	Option<Path> resolve_import(Path from, const ImportAst& i, PathCache& paths) {
		if (!i.n_parents.has()) throw "todo: global resolution";
		return paths.resolve(from, RelPath { i.n_parents.get(), i.path });
	}

	void parse_everything(Grow<FileAst>& out, List<Diagnostic>::Builder& diagnostics, Arena& diags_arena, Path first_path, Arena& ast_arena, DocumentProvider& document_provider, PathCache& path_cache) {
		MaxSizeVector<16, Path> to_parse;
		to_parse.push(first_path);
		MaxSizeSet<32, Path, Path::hash> enqued_set; // Set of paths that are either already parsed, or in to_parse.
		enqued_set.must_insert(first_path);

		do {
			Path path = to_parse.pop_and_return();
			Option<StringSlice> document = document_provider.try_get_document(path, NZ_EXTENSION, ast_arena);
			if (!document.has()) throw "todo: no such file";

			try {
				FileAst& f = out.push({ path, document.get() });
				parse_file(f, path_cache, ast_arena);
			} catch (ParseDiagnostic diag) {
				diagnostics.add({ path, diag }, diags_arena);
				return;
			}

			for (const ImportAst& i : out.back().imports) {
				Option<Path> op_dependency_path = resolve_import(path, i, path_cache);
				if (!op_dependency_path.has()) throw "todo";
				Path dependency_path = op_dependency_path.get();

				if (enqued_set.try_insert(dependency_path).was_added)
					to_parse.push(dependency_path);
			}
		} while (!to_parse.empty());
	}

	using Compiled = MaxSizeMap<32, Path, ref<const Module>, Path::hash>;

	Option<Arr<ref<const Module>>> get_imports(
		const Arr<ImportAst>& imports, Path cur_path, const Compiled& compiled, PathCache& paths, Arena& arena, List<Diagnostic>::Builder& diagnostics
	) {
		return map_or_fail<ref<const Module>>()(arena, imports, [&](const ImportAst& ast) {
			Path p = resolve_import(cur_path, ast, paths).get(); // Should succeed because we already did this in parse_everything
			Option<const ref<const Module>&> m = compiled.get(p);
			if (!m.has()) {
				diagnostics.add({ cur_path, ast.range, Diag::Kind::CircularImport }, arena);
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
	List<Diagnostic>::Builder diagnostics;
	parse_everything(parsed, diagnostics, out.arena, first_path, ast_arena, document_provider, out.paths);
	if (diagnostics.empty()) {
		// Go in reverse order -- if we ever see some dependency that's not compiled yet, it indicates a circular dependency.
		Compiled compiled;
		Option<Arr<Module>> modules = map_or_fail_reverse<Module>()(out.arena, parsed, [&](const FileAst& ast, ref<Module> m) {
			Option<Arr<ref<const Module>>> imports = get_imports(ast.imports, ast.path, compiled, out.paths, out.arena, diagnostics);
			m->path = ast.path;
			m->imports = imports.get();
			m->comment = ast.comment.has() ? Option { str(out.arena, ast.comment.get()) } : Option<ArenaString> {};
			if (!imports.has()) {
				assert(!out.diagnostics.empty());
				return false;
			}
			check(m, ast, out.arena, diagnostics);
			if (!diagnostics.empty())
				return false;
			compiled.must_insert(m->path, m);
			return true;
		});
		if (modules.has())
			out.modules = modules.get();
	}
	out.diagnostics = diagnostics.finish();
}
