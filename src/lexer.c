#include "tokens.h"
#include "xft.h"

__attribute__((noinline))
t_result	lex_next_token(t_buffer src, s_lexer_state *__restrict__ const state, t_vec *__restrict__ const vec)
{
	
	static const t_any states[] = {
		// [0 ... UINT8_MAX] = &&unexpected_char,	
		// [','] = &&single_char_token,
	};

	t_token		tok;

	tok.type	= TOK_EOF;
	tok.as.lexeme.start = state->offset;

	t_u64a current = src.mem[state->offset];

	identifier:

	numeric_constant:

	string_literal:

	possible_operator_followed_by_eq_or: // +
		t_u32a idx =  xft_tern(state->offset + 1 < src.mem, state->offset + 1, state->offset);
		t_u64a next = xft_tern(state->offset + 1 < src.mem, src.mem[idx], 0);
		t_token_type type = xft_tern(next == "=", TOK_ADDEQ, TOK_ADD);
		t_u64a newOffset = xft_tern(next == "=", 2, 1);

		state->offset += newOffset; // Leer caracteres
		tok.type = type;
		goto success;
	operator_followed_by_equal:

	double_char_token:

	single_char_token:

	div_or_comment:

	success:

	return (OK);

	unexpected_char:
}
