#pragma once

#include <xft.h>
#include "tokens.h"
#include "errors.h"

#ifndef JSPDL_USE_SIMD

typedef struct s_lexer_state {
	t_u32a						offset;
	t_indexmap	*__restrict__	symbol_table;
	t_soa	*__restrict__		symbol_dense_array;
} t_lexer_state;

#else

typedef struct e_last_bits_ctx {
	t_u8 whitespace			: 1;
	t_u8 identifiers		: 1;
	t_u8 numeric			: 1;
	t_u8 equals				: 1;
	t_u8 ampers				: 1;
	t_u8 pipes				: 1;
	t_u8 ops_possible_eq	: 1;
	t_u8 slashes			: 1;
}	__attribute__((packed))	t_last_bits_ctx;

typedef struct s_lexer_state {
	t_u32a						offset;
	t_indexmap	*__restrict__	symbol_table;
	t_soa	*__restrict__		symbol_dense_array;
	t_last_bits_ctx				last_bits;
} t_lexer_state;

#endif

t_result	lex_next_token(
	t_allocator							allocator,
	const t_buffer						src,
	const t_buffer						file_name,
	t_lexer_state	*__restrict__ const	state,
	t_soa			*__restrict__ const	soa,
	t_error_state	*__restrict__ const	error_state
) __attribute__((noinline, __nonnull__(4, 5, 6)));
