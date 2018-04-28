#pragma once

#include "./diag/diag.h"
#include "./model/model.h"
#include "../host/DocumentProvider.h"

struct CompiledProgram {
	Arena arena;
	PathCache paths;
	Vec<ref<Module>> modules;
	Vec<Diagnostic> diagnostics;
};

extern const StringSlice NZ_EXTENSION;

void compile(CompiledProgram& out, DocumentProvider& document_provider, ref<const Path> first_path);
