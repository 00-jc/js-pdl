#pragma once

#include <xft.h>
#include "tokens.h"
#include "errors.h"

typedef struct s_lexer_state {
	t_u32a						offset;
} t_lexer_state;

t_result	pdl_lex_next_token(
	t_allocator							allocator,
	const t_buffer						src,
	const t_buffer						file_name,
	t_lexer_state	*__restrict__ const	state,
	t_soa			*__restrict__ const	soa,
	t_error_state	*__restrict__ const	error_state
) __attribute__((noinline, __nonnull__(4, 5, 6)));
