#pragma once

#include "../util/store/List.h"
#include "../util/PathCache.h"
#include "../host/DocumentProvider.h"
#include "./diag/diag.h"
#include "./model/BuiltinTypes.h"
#include "./model/model.h"

struct CompiledProgram {
	Arena arena;
	PathCache paths;
	Slice<Module> modules;
	List<Diagnostic> diagnostics;
	BuiltinTypes builtin_types;
};

extern const StringSlice NZ_EXTENSION;

void compile(CompiledProgram& out, DocumentProvider& document_provider, Path first_path);
