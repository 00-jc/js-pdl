#pragma once

#include <xft.h>

typedef struct s_symbol
{
	t_u32a		scope;
}	t_symbol;

enum e_symbol_soa_field {
	SYMBOL_SOA_SCOPE,
};

#define SYMBOL_SOA_SCOPE_SIZE		sizeof(((t_symbol *)0)->scope)

extern const t_soa_context	symbol_soa_ctx;
