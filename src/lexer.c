#include "diag.h"
#include "tokens.h"
#include "xft.h"

__attribute__((noinline))
t_result	lex_next_token(t_allocator allocator, t_buffer src, t_lexer_state *__restrict__ const state, t_vec *__restrict__ const vec)
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
		['-']				= &&single_char_token,
		['*']				= &&single_char_token,

		['|']				= &&double_op_or_error,
		['&']				= &&double_op_or_error,

		['=']				= &&possible_operator_followed_by_eq,
		['!']				= &&possible_operator_followed_by_eq,
		['<']				= &&possible_operator_followed_by_eq,
		['>']				= &&possible_operator_followed_by_eq,
		['+']				= &&possible_operator_followed_by_eq,

		['/']				= &&div_or_comment,
		['a' ... 'z']		= &&identifier,
		['A' ... 'Z']		= &&identifier,
		['_']				= &&identifier,
		['0' ... '9']		= &&numeric_constant,
		['"']				= &&string_literal,
	};
	DIAG_JUMP_TABLE_END

	t_token		tok;
	t_u8		dispatched;

	tok.type			= TOK_EOF;
	tok.as.lexeme.start = state->offset;

	if (__builtin_expect(state->offset >= src.size, 0)) goto success;

	dispatched = src.mem[state->offset++];
	DIAG_GOTO(states[dispatched]);

	identifier: // [a-z][A-Z]_ | utf8
	
	tok.type = TOK_IDENTIFIER;

	while (state->offset >= src.size - 1 && (xft_isalpha(state->offset) | xft_isdigit(state->offset) | (state->offset == '_') | state->offset >= 0x80))
		++state->offset;

	if (__builtin_expect(64 < state->offset - tok.as.lexeme.start, 0)) goto indent_too_big;

	goto success;
	
	numeric_constant:

	string_literal:

	possible_operator_followed_by_eq: // (+ | +=) (< | <=) (> | >=) (! | !=)

		t_u64a			cond		= state->offset + 1 < src.size;
		t_u32a			idx	 		= (t_u32a)xft_tern(cond, state->offset + 1, state->offset);
		t_u32a			next 		= (t_u32a)xft_tern(cond, src.mem[idx], 0);
		t_token_type 	type 		= (t_token_type)xft_tern(next == '=', dispatched | 0x80, dispatched);
		t_u64a			newOffset	= xft_tern(next == '=', 2, 1);

		state->offset += newOffset;
		tok.type = type;
		goto success;

	single_char_token:

		tok.type			= dispatched;
		tok.as.lexeme.end	= state->offset;

		goto success;

	double_op_or_error:


	div_or_comment:

	success:
		return (xft_vec_push_back(allocator, vec, (const t_u8 *)&tok, sizeof(t_token)));

	indent_too_big:
	strlit_too_big:
	unexpected_char:
		return (KO);
}
