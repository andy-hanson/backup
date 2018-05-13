#include "./compile.h"

#include "../util/store/ListBuilder.h"
#include "../util/store/MaxSizeMap.h"
#include "../util/store/MaxSizeSet.h"
#include "./check/check.h"
#include "./parse/parser.h"

namespace {
	template <typename Out>
	struct map_or_fail_reverse {
		template <uint elements_per_node, typename In, typename /*const In&, uninitialized T* => Out*/ Cb>
		Option<Slice<Out>> operator()(Arena& arena, const BlockedList<elements_per_node, In>& inputs, Cb cb) {
			Slice<Out> out = uninitialized_array<Out>(arena, inputs.size());
			uint i = 0;
			bool success = true;
			inputs.each_reverse([&](const In& input) {
				if (!success) return;
				success = cb(input, &out[i]);
				++i;
			});
			if (success) {
				assert(i == out.size());
				return Option { out };
			} else
				return {};
		}
	};

	Option<Path> resolve_import(Path from, const ImportAst& i, PathCache& paths) {
		if (!i.n_parents.has()) todo(); // global import resolution
		return paths.resolve(from, RelPath { i.n_parents.get(), i.path });
	}

	BlockedList<4, FileAst> parse_everything(ListBuilder<Diagnostic>& diagnostics, Arena& diags_arena, Path first_path, Arena& ast_arena, DocumentProvider& document_provider, PathCache& path_cache) {
		BlockedList<4, FileAst> out;
		MaxSizeVector<16, Path> to_parse;
		to_parse.push(first_path);
		MaxSizeSet<32, Path, Path::hash> enqued_set; // Set of paths that are either already parsed, or in to_parse.
		enqued_set.must_insert(first_path);

		do {
			Path path = to_parse.pop_and_return();
			Option<StringSlice> document = document_provider.try_get_document(path, NZ_EXTENSION, ast_arena);
			if (!document.has()) todo(); // Imported from a file that doesn't exist

			try {
				FileAst& f = out.push({ path, document.get() }, ast_arena);
				parse_file(f, path_cache, ast_arena);
			} catch (ParseDiagnostic diag) {
				diagnostics.add({ path, diag }, diags_arena);
				return {};
			}

			for (const ImportAst& i : out.back().imports) {
				Option<Path> op_dependency_path = resolve_import(path, i, path_cache);
				if (!op_dependency_path.has()) todo(); // resolution failed
				Path dependency_path = op_dependency_path.get();

				if (enqued_set.try_insert(dependency_path).was_added)
					to_parse.push(dependency_path);
			}
		} while (!to_parse.is_empty());

		return out;
	}

	using Compiled = MaxSizeMap<32, Path, Ref<const Module>, Path::hash>;

	Option<Slice<Ref<const Module>>> get_imports(
		const Slice<ImportAst>& imports, Path cur_path, const Compiled& compiled, PathCache& paths, Arena& arena, ListBuilder<Diagnostic>& diagnostics
	) {
		return map_or_fail<Ref<const Module>>()(arena, imports, [&](const ImportAst& ast) {
			Path p = resolve_import(cur_path, ast, paths).get(); // Should succeed because we already did this in parse_everything
			Option<const Ref<const Module>&> m = compiled.get(p);
			if (!m.has()) {
				diagnostics.add({ cur_path, ast.range, Diag::Kind::CircularImport }, arena);
				return Option<Ref<const Module>>{};
			}
			return Option { m.get() };
		});
	}
}

const StringSlice NZ_EXTENSION = "nz";

// Note: if there are any diagnostics, 'out' should not be used for anything other than printing them.
void compile(CompiledProgram& out, DocumentProvider& document_provider, Path first_path) {
	Arena ast_arena;
	ListBuilder<Diagnostic> diagnostics;
	auto parsed = parse_everything(diagnostics, out.arena, first_path, ast_arena, document_provider, out.paths);
	if (diagnostics.is_empty()) {
		// Go in reverse order -- if we ever see some dependency that's not compiled yet, it indicates a circular dependency.
		Compiled compiled;
		Option<BuiltinTypes> builtin_types;
		Option<Slice<Module>> modules = map_or_fail_reverse<Module>()(out.arena, parsed, [&](const FileAst& ast, Ref<Module> m) {
			Option<Slice<Ref<const Module>>> imports = get_imports(ast.imports, ast.path, compiled, out.paths, out.arena, diagnostics);
			if (!imports.has()) {
				assert(!diagnostics.is_empty());
				return false;
			}
			m->path = ast.path;
			m->imports = imports.get();
			m->comment = ast.comment.has() ? Option { copy_string(out.arena, ast.comment.get()) } : Option<ArenaString> {};
			check(m, builtin_types, ast, out.arena, diagnostics);
			if (!diagnostics.is_empty())
				return false;
			compiled.must_insert(m->path, m);
			return true;
		});
		if (modules.has())
			out.modules = modules.get();
		out.builtin_types = builtin_types.get();
	}
	out.diagnostics = diagnostics.finish();
}
