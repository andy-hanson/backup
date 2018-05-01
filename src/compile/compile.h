#pragma once

#include "./diag/diag.h"
#include "./model/model.h"
#include "../host/DocumentProvider.h"
#include "../util/Grow.h"

struct CompiledProgram {
	Arena arena;
	PathCache paths;
	Arr<Module> modules;
	Grow<Diagnostic> diagnostics;
};

extern const StringSlice NZ_EXTENSION;

void compile(CompiledProgram& out, DocumentProvider& document_provider, Path first_path);
