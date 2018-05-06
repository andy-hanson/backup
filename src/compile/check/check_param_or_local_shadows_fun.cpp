#include "./check_param_or_local_shadows_fun.h"

void check_param_or_local_shadows_fun(CheckCtx& al, const StringSlice& name, const FunsTable& funs_table, const Slice<SpecUse>& current_specs) {
	if (funs_table.has(name))
		al.diag(name, Diag::Kind::LocalShadowsFun);
	for (const SpecUse& spec_use : current_specs)
		for (const FunSignature& sig : spec_use.spec->signatures)
			if (sig.name == name)
				al.diag(name,Diag::Kind::LocalShadowsSpecSig);
}
