#include "tokens.h"
#include "xft.h"

__attribute__((noinline))
t_result	lex_next_token(t_buffer src, t_vec *__restrict__ const vec)
{
	
	static const t_any states[] = {
		// [0 ... UINT8_MAX] = &&unexpected_char,	
		// [','] = &&single_char_token,
	};

	t_token		tok;

	tok.type	= TOK_EOF;

	identifier:

	numeric_constant:

	string_literal:

	possible_operator_followed_by_eq_or_double: // +, -

	operator_followed_by_equal:

	double_char_token:

	single_char_token:

	div_or_comment:

	success:

	return (OK);

	unexpected_char:
}
