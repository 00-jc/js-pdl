#pragma once

#include <xft.h>
#include "tokens.h"
#include "errors.h"

#ifndef LEXER_USE_SIMD

typedef struct s_lexer_state {
	t_u32a						offset;
	t_indexmap	*__restrict__	symbol_table;
	t_soa	*__restrict__		symbol_dense_array;
} t_lexer_state;

#else

// carries across simd frames, packed in a single byte.
typedef struct s_lexer_carry {
	t_u8	comment_open	: 1;	// last byte is inside a comment
	t_u8	string_open		: 1;	// last byte is inside a string literal
	t_u8	ident_open		: 1;	// last byte is an identifier char
	t_u8	num_open		: 1;	// last byte is a numeral char
	t_u8	opeq_pend		: 1;	// first byte is the '=' of a '<=' '>=' '+=' started last frame
	t_u8	eqeq_pend		: 1;	// first byte is the 2nd '=' of a '==' started last frame
	t_u8	and_pend		: 1;	// first byte is the 2nd '&' of a '&&' started last frame
	t_u8	or_pend			: 1;	// first byte is the 2nd '|' of a '||' started last frame
	t_u8	backsl_pend		: 1;	// first byte is the 2nd '\' of a '\\' started last frame
} t_lexer_carry;

typedef struct s_lexer_state {
	t_u32a						offset;
	t_u32a						relative;
	t_indexmap	*__restrict__	symbol_table;
	t_soa	*__restrict__		symbol_dense_array;
	t_nmask						starts;
	t_nmask						ends;
	t_lexer_carry				carry;
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
