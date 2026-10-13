#include <xft.h>
#include "diag.h"
#include "lexer.h"

DIAG_FAM_INIT_BEGIN
const t_soa_context	token_soa_ctx = {
	.total_struct_size	= TOKEN_SOA_TYPE_SIZE + TOKEN_SOA_AS_SIZE + TOKEN_SOA_SYMBOL_SIZE,
	.max_alignment		= alignof(t_token),
	.fields_count		= TOKEN_SOA_FIELDS_COUNT,
	.entries			= {
		[TOKEN_SOA_TYPE] = {
			.size						= TOKEN_SOA_TYPE_SIZE,
			.offset_from_struct_base	= __builtin_offsetof(t_token, type),
			.size_of_prev_fields		= 0,
		},
		[TOKEN_SOA_AS] = {
			.size						= TOKEN_SOA_AS_SIZE,
			.offset_from_struct_base	= __builtin_offsetof(t_token, as),
			.size_of_prev_fields		= TOKEN_SOA_TYPE_SIZE,
		},
		[TOKEN_SOA_SYMBOL] = {
			.size						= TOKEN_SOA_SYMBOL_SIZE,
			.offset_from_struct_base	= __builtin_offsetof(t_token, symbol),
			.size_of_prev_fields		= TOKEN_SOA_TYPE_SIZE + TOKEN_SOA_AS_SIZE,
		},
	},
};
DIAG_FAM_INIT_END

__attribute__((noinline, __nonnull__(4, 5, 6)))
t_result	lex_next_token(
	t_allocator							allocator,
	const t_buffer						src,
	const t_buffer						file_name,
	t_lexer_state	*__restrict__ const	state,
	t_soa			*__restrict__ const	soa,
	t_error_state	*__restrict__ const	error_state
)
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
	t_buffer	fmt;
	t_u64		args[4];
	t_u32a		line;

whitespace:
	if (__builtin_expect(state->offset >= src.size, 0)) goto success;
	dispatched = src.mem[state->offset++];
	if (xft_isspace(dispatched)) goto whitespace;

	tok.type			= TOK_EOF;
	tok.as.lexeme.start = state->offset - 1;

	DIAG_GOTO(states[dispatched]);

	identifier: // [a-z][A-Z]_ | utf8
	{
		tok.type = TOK_IDENTIFIER;
		while (state->offset < src.size && (xft_isalpha(src.mem[state->offset]) | xft_isdigit(src.mem[state->offset]) | (src.mem[state->offset] == '_') | (src.mem[state->offset] >= 0x80)))
			++state->offset;
		if (__builtin_expect(64 < state->offset - tok.as.lexeme.start, 0)) goto indent_too_big;
		tok.as.lexeme.end = state->offset;
		goto success;
	}
	
	numeric_constant:
	{
		tok.type = TOK_CONST_INT;
		while (state->offset < src.size && xft_isdigit(src.mem[state->offset]))
			++state->offset;
		if (state->offset < src.size && src.mem[state->offset] == '.')
		{
			++state->offset;
			goto float_constant;
		}
		// TODO: atoi
		tok.as.lexeme.end = state->offset;
		tok.as.value = 0;
		goto success;
	}

	float_constant:
	{
		[[maybe_unused]] t_u32a real_part_start = tok.as.lexeme.start;
		[[maybe_unused]] t_u32a real_part_end = state->offset - 1;
		[[maybe_unused]] t_u32a decimal_start = state->offset;
		tok.type = TOK_CONST_FLOAT;
		while (state->offset < src.size && xft_isdigit(src.mem[state->offset]))
			++state->offset;
		// TODO: atof
		tok.as.lexeme.end = state->offset;
		tok.as.value = 0;
		goto success;
	}

	string_literal:
	{
		tok.type = TOK_STRING_LITERAL;
		tok.as.lexeme.start = state->offset;
		while (1)
		{
			strlit_loop:
			if (__builtin_expect(state->offset >= src.size, 0)) goto error_unterminated_string;
			if (state->offset < src.size && src.mem[state->offset] == '\\')
			{
				state->offset += 2;
				goto strlit_loop;
			}
			if (state->offset < src.size && src.mem[state->offset] == '"')
			{
				tok.as.lexeme.end = state->offset++;
				break;
			}
			++state->offset;
		}
		if (__builtin_expect(state->offset - tok.as.lexeme.start > 64, 0)) goto strlit_too_big;
		goto success;
	}

	possible_operator_followed_by_eq: // (+ | +=) (< | <=) (> | >=) (! | !=)
	{
		t_u64a			cond		= state->offset < src.size;
		t_u32a			idx	 		= (t_u32a)xft_tern(cond, state->offset, state->offset - 1);
		t_u32a			next 		= (t_u32a)xft_tern(cond, src.mem[idx], 0);
		t_token_type 	type 		= (t_token_type)xft_tern(next == '=', dispatched | 0x80, dispatched);
		t_u64a			newOffset	= xft_tern(next == '=', 1, 0);

		state->offset += newOffset;
		tok.as.lexeme.end = state->offset;
		tok.type = type;
		goto success;
	}
	single_char_token:
	{
		tok.type			= dispatched;
		tok.as.lexeme.end	= state->offset;
		goto success;
	}
	double_op_or_error:
	{

	}
	div_or_comment:
	{

	}
	success:
		return (xft_soa_append_from_struct(allocator, soa, &tok, &token_soa_ctx));

#define LEX_ERR_FMT(msg) "error in %s:%u:%u: " msg "\n"
#define LEX_ERR(msg) xft_fatptr((t_any)LEX_ERR_FMT(msg), sizeof(LEX_ERR_FMT(msg)) - 1)

	error_unterminated_string:
		fmt = LEX_ERR("unterminated string");
		goto report_error;
	indent_too_big:
		fmt = LEX_ERR("identifier too big");
		goto report_error;
	strlit_too_big:
		fmt = LEX_ERR("string literal too big");
		goto report_error;
	unexpected_char:
		fmt = LEX_ERR("unrecognized char");
		goto report_error;

#undef LEX_ERR
#undef LEX_ERR_FMT

	report_error:
		line = line_of_token(error_state, tok.as.lexeme.start);
		args[0] = (t_u64)(t_uptr)file_name.mem;
		args[1] = file_name.size;
		args[2] = line;
		args[3] = tok.as.lexeme.start - ((const t_u32a *)error_state->line_offsets.buf.mem)[line - 1] + 1;
		xft_fmt_writer(error_state->writer, fmt, args);
		return (KO);
}
