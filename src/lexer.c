#include "diag.h"
#include "tokens.h"
#include "xft.h"

__attribute__((noinline))
t_result	lex_next_token(t_allocator allocator, t_buffer src, t_lexer_state* __restrict__ const state, t_vec* __restrict__ const vec)
{

	DIAG_JUMP_TABLE_BEGIN
		static const t_any states[] = {
			[0 ... UINT8_MAX] = && unexpected_char,
			[0] = && success,

			[' '] = && whitespace,
			['\t'] = && whitespace,
			['\r'] = && whitespace,
			['\n'] = && whitespace,

			[','] = && single_char_token,
			[';'] = && single_char_token,
			['('] = && single_char_token,
			[')'] = && single_char_token,
			['{'] = && single_char_token,
			['}'] = && single_char_token,
			['-'] = && single_char_token,
			['*'] = && single_char_token,

			['|'] = && double_op_or_error,
			['&'] = && double_op_or_error,

			['='] = && possible_operator_followed_by_eq,
			['!'] = && possible_operator_followed_by_eq,
			['<'] = && possible_operator_followed_by_eq,
			['>'] = && possible_operator_followed_by_eq,
			['+'] = && possible_operator_followed_by_eq,

			['/'] = && div_or_comment,
			['a' ... 'z'] = && identifier,
			['A' ... 'Z'] = && identifier,
			['_'] = && identifier,
			['0' ... '9'] = && numeric_constant,
			['"'] = && string_literal,
	};
	DIAG_JUMP_TABLE_END

		t_token		tok;
	t_u8		dispatched;

restart:
	tok.type = TOK_EOF;
	tok.as.lexeme.start = state->offset;

	if (__builtin_expect(state->offset >= src.size, 0)) goto success;

	dispatched = src.mem[state->offset++];
	DIAG_GOTO(states[dispatched]);

whitespace:
	while (state->offset < src.size && (src.mem[state->offset] == ' ' ||
		src.mem[state->offset] == '\t' ||
		src.mem[state->offset] == '\r' ||
		src.mem[state->offset] == '\n'))
	{
		++state->offset;
	}
	goto restart;

identifier: // [a-z][A-Z]_ | utf8
	{
		tok.type = TOK_IDENTIFIER;

		while (state->offset < src.size && (xft_isalpha(src.mem[state->offset]) | xft_isdigit(src.mem[state->offset]) | (src.mem[state->offset] == '_') |
			((t_u8)src.mem[state->offset] >= 0x80)))
			++state->offset;

		tok.as.lexeme.end = state->offset;

		if (__builtin_expect(64 < state->offset - tok.as.lexeme.start, 0)) goto indent_too_big;

		goto success;
	}

numeric_constant:
	goto unexpected_char;

string_literal:
	goto unexpected_char;

possible_operator_followed_by_eq: // (+ | +=) (< | <=) (> | >=) (! | !=)
	{
		t_u64a	cond = state->offset < src.size;
		t_u8	next = (t_u8)xft_tern(cond, src.mem[state->offset], 0);

		if (next == '=')
		{
			++state->offset;
			switch (dispatched)
			{
			case '=': tok.type = TOK_EQEQ; break;
			case '!': tok.type = TOK_NE; break;
			case '<': tok.type = TOK_LE; break;
			case '>': tok.type = TOK_GE; break;
			case '+': tok.type = TOK_ADDEQ; break;
			default: tok.type = dispatched; break;
			}
		}
		else
			tok.type = dispatched;

		tok.as.lexeme.end = state->offset;
		goto success;

	}

	single_char_token:
		tok.type			= dispatched;
		tok.as.lexeme.end	= state->offset;
		goto success;

	double_op_or_error:
		{
			if (__builtin_expect(state->offset >= src.size || src.mem[state->offset] != dispatched, 0))
				goto unexpected_char;
			
			++state->offset;
			tok.as.lexeme.end = state->offset;
			tok.type = (dispatched == '&') ? TOK_AND : TOK_OR;
			goto success;
		}

	div_or_comment:
		{
			if (state->offset < src.size && src.mem[state->offset] == '/')
			{
				++state->offset;
				while (state->offset < src.size && src.mem[state->offset] != '\n')
					++state->offset;
				goto restart;
			}
			if (state->offset < src.size && src.mem[state->offset] == '*')
			{
				++state->offset;
				while (state->offset + 1 < src.size &&
					!(src.mem[state->offset] == '*' && src.mem[state->offset + 1] == '/'))
				{
					++state->offset;
				}
				if (__builtin_expect(state->offset + 1 >= src.size, 0)) goto unexpected_char;
				state->offset += 2;
				goto restart;
			}
			tok.type = dispatched;
			tok.as.lexeme.end = state->offset;
			goto success;
		}

	success:
		return (xft_vec_push_back(allocator, vec, (const t_u8 *)&tok, sizeof(t_token)));

	indent_too_big:
	//strlit_too_big:
	unexpected_char:
		return (KO);
}
