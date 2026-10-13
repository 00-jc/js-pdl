#include "symbols.h"
#include "diag.h"

DIAG_FAM_INIT_BEGIN
const t_soa_context	symbol_soa_ctx = {
	.fields_count = 1,
	.max_alignment = alignof(t_symbol),
	.total_struct_size = SYMBOL_SOA_SCOPE_SIZE,
	.entries = {
		[SYMBOL_SOA_SCOPE] = {
			.offset_from_struct_base = __builtin_offsetof(t_symbol, scope),
			.size = SYMBOL_SOA_SCOPE_SIZE,
			.size_of_prev_fields = 0,
		},
	},
};
DIAG_FAM_INIT_END
