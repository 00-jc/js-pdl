#include "diag.h"
#include "tokens.h"
#include "xft.h"

__attribute__((noinline))
t_result	lex_next_token(t_buffer src, s_lexer_state *__restrict__ const state, t_vec *__restrict__ const vec)
{
	
	DIAG_JUMP_TABLE_BEGIN
	static const t_any states[] = {
		[0 ... UINT8_MAX]	= &&unexpected_char,
		[0]					= &&success,
		[',']				= &&single_char_token,
		[';']				= &&single_char_token,
		['(']				= &&single_char_token,
		[')']				= &&single_char_token,
		['{']				= &&single_char_token,
		['}']				= &&single_char_token,
		['<']				= &&operator_followed_by_equal,
		['>']				= &&operator_followed_by_equal,
		['+']				= &&possible_operator_followed_by_eq_or_double,
		['-']				= &&possible_operator_followed_by_eq_or_double,
		['*']				= &&operator_followed_by_equal,
		['/']				= &&div_or_comment,
		['a' ... 'z']		= &&identifier,
		['A' ... 'Z']		= &&identifier,
		['_']				= &&identifier,
		['0' ... '9']		= &&numeric_constant,
		['"']				= &&string_literal,
	};
	DIAG_JUMP_TABLE_END

	t_token		tok;

	tok.type	= TOK_EOF;

	DIAG_GOTO(states[src.mem[state->offset++]]);

	identifier:

	numeric_constant:

	string_literal:

	possible_operator_followed_by_eq_or_double: // +, -

	operator_followed_by_equal:

	single_char_token:

	div_or_comment:

	success:
		return (xft_vec_push_back(allocator, vec, (const t_u8 *)&tok, sizeof(t_token)));

	unexpected_char:
	return (KO);
}
