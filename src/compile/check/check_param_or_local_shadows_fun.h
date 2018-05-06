#pragma once

#include "./CheckCtx.h"

// current_specs: the specs from the current function.
void check_param_or_local_shadows_fun(CheckCtx& al, const StringSlice& name, const FunsTable& funs_table, const Slice<SpecUse>& current_specs);
