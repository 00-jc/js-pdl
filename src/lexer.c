#include <xft.h>
#include "diag.h"
#include "lexer.h"

#if XFT_HAS_512_VEC

# define HAS_VECS
# define NATIVE_VEC			t_vu512a
# define NATIVE_VEC_PTR		t_blk512r
# define VEC_MASK			t_u64
# define SPLAT(x)			xft_vsplat512((x))
# define MASK(x)			xft_vmask512((x))
# define RANGEMASK(x, y, z) xft_rangemask512((x), (y), (z))
# define CTZ(x)				xft_memctz_u64((x))
# define POPCNT(x)			xft_popcount_u64((x))

#elif XFT_HAS_256_VEC

# define HAS_VECS
# define NATIVE_VEC			t_vu256a
# define NATIVE_VEC_PTR		t_blk256r
# define VEC_MASK			t_u32
# define SPLAT(x)			xft_vsplat256((x))
# define MASK(x)			xft_vmask256((x))
# define RANGEMASK(x, y, z) xft_rangemask256((x), (y), (z))
# define CTZ(x)				xft_memctz_u32((x))
# define POPCNT(x)			xft_popcount_u32((x))

#elif XFT_HAS_128_VEC

# define HAS_VECS
# define NATIVE_VEC			t_vu128a
# define NATIVE_VEC_PTR		t_blk128r
# define VEC_MASK			t_u16
# define SPLAT(x)			xft_vsplat128((x))
# define MASK(x)			xft_vmask128((x))
# define RANGEMASK(x, y, z) xft_rangemask128((x), (y), (z))
# define CTZ(x)				xft_memctz_u16((x))
# define POPCNT(x)			xft_popcount_u32((x) | (t_u32)0)

#endif

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

__attribute__((__nonnull__(1), pure, __always_inline__, __unused__))
static inline t_u64a		interpret_numeral(const t_u8 *__restrict__ const mem, t_u32a len)
{
	t_u64a	value;
	t_size	it;

	value = 0;
	it = 0;
	while (it < len) value = (value << 3) + (value << 1) + (mem[it++] - '0');
	return value;
}

__attribute__((__nonnull__(1), pure, __always_inline__, __unused__))
static inline t_f64a		interpret_decimal_part(const t_u8 *__restrict__ const mem, t_u32a len)
{
	t_f64a	value;
	t_size	it;

	value = 0;
	it = len;
	while (it > 0) value = (value + (mem[--it] - '0')) / 10;
	return value;
}

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
		['%']				= &&single_char_token,

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
		[0x80 ... UINT8_MAX]= &&identifier,
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
	tok.type			= TOK_EOF;
	tok.as.value.i		= 0;
	tok.symbol			= 0;
#ifdef HAS_VECS 
		vector_ws_loop: if (src.size  - state->offset >= sizeof(NATIVE_VEC))
		{
			NATIVE_VEC x = *(NATIVE_VEC_PTR)(src.mem + state->offset);
			VEC_MASK end = (VEC_MASK)~(MASK(x == ' ') | RANGEMASK(x, '\t', '\r'));
			if (__builtin_expect(end != 0, 1))
				state->offset += CTZ(end);
			else {
				state->offset += sizeof(NATIVE_VEC);
				goto vector_ws_loop;
			}
		}
#endif
whitespace_scalar:
	if (__builtin_expect(state->offset >= src.size, 0)) goto success;
	dispatched = src.mem[state->offset++];
	if (xft_isspace(dispatched)) goto whitespace_scalar;

	tok.as.lexeme.start = state->offset - 1;

	DIAG_GOTO(states[dispatched]);

	identifier: // [a-z][A-Z]_ | utf8
	{
		tok.type = TOK_IDENTIFIER;
#ifdef HAS_VECS 
		vector_id_loop: if (src.size  - state->offset >= sizeof(NATIVE_VEC))
		{
			NATIVE_VEC x = *(NATIVE_VEC_PTR)(src.mem + state->offset);
			VEC_MASK mask = RANGEMASK(x | 0x20, 'a', 'z') | MASK(x == '_') | RANGEMASK(x, 0x80, 0xFF) | RANGEMASK(x, '0', '9');
			VEC_MASK end = (VEC_MASK)~mask;
			if (__builtin_expect(end != 0, 1)) {
				state->offset += CTZ(end);
				goto kw_selection;
			}
			state->offset += sizeof(NATIVE_VEC);
			goto vector_id_loop;
		}
#endif
		while (state->offset < src.size && (xft_isalpha(src.mem[state->offset]) | xft_isdigit(src.mem[state->offset]) | (src.mem[state->offset] == '_') | (src.mem[state->offset] >= 0x80)))
			++state->offset;
		kw_selection:
		if (__builtin_expect(64 < state->offset - tok.as.lexeme.start, 0)) goto ident_too_big;
		tok.as.lexeme.end = state->offset;
		t_u32a size = tok.as.lexeme.end - tok.as.lexeme.start;
		if (size <= 8)
		{
			t_u64a x = 0;
			xft_memcpy(&x, src.mem + tok.as.lexeme.start, size);
			x = xft_from_le64(x);
		#define TEST_FOR_KW(kw_name, le_value) tok.type = (t_u8)xft_tern(x == (le_value), (kw_name), tok.type);
			// used: https://codeberg.org/00jc/repr
			TEST_FOR_KW(TOK_TRUE,	0x65757274);
			TEST_FOR_KW(TOK_FALSE,	0x65736C6166);
			TEST_FOR_KW(TOK_LET,	0x74656C);
			TEST_FOR_KW(TOK_INT,	0x746E69);
			TEST_FOR_KW(TOK_FLOAT,	0x74616F6C66);
			TEST_FOR_KW(TOK_BOOL,	0x6E61656C6F6F62);
			TEST_FOR_KW(TOK_STRING,	0x676E69727473);
			TEST_FOR_KW(TOK_VOID,	0x64696F76);
			TEST_FOR_KW(TOK_RETURN,	0x6E7275746572);
			TEST_FOR_KW(TOK_IF,		0x6669);
			TEST_FOR_KW(TOK_DO,		0x6F64);
			TEST_FOR_KW(TOK_WHILE,	0x656C696877);
			TEST_FOR_KW(TOK_BREAK,	0x6B61657262);
			TEST_FOR_KW(TOK_FUNC,	0x6E6F6974636E7566);
			TEST_FOR_KW(TOK_PROMPT,	0x74706D6F7270);
			TEST_FOR_KW(TOK_ECHO,	0x6F686365);
		#undef TEST_FOR_KW
		}
		goto success;
	}
	
	numeric_constant:
	{
		tok.type = TOK_CONST_INT;

#ifdef HAS_VECS 
		vector_int_loop: if (src.size  - state->offset >= sizeof(NATIVE_VEC))
		{
			NATIVE_VEC x = *(NATIVE_VEC_PTR)(src.mem + state->offset);
			VEC_MASK end = (VEC_MASK)~RANGEMASK(x, '0', '9');
			if (__builtin_expect(end != 0, 1))
				state->offset += CTZ(end);
			else {
				state->offset += sizeof(NATIVE_VEC);
				goto vector_int_loop;
			}
		}
#endif
		while (state->offset < src.size && xft_isdigit(src.mem[state->offset]))
			++state->offset;
		if (state->offset < src.size && src.mem[state->offset] == '.')
		{
			++state->offset;
			goto float_constant;
		}
		tok.as.value.i = interpret_numeral(src.mem + tok.as.lexeme.start, state->offset - tok.as.lexeme.start);
		goto success;
	}

	float_constant:
	{
		t_u32a real_part_start = tok.as.lexeme.start;
		t_u32a real_part_end = state->offset - 1;
		t_u32a decimal_start = state->offset;
		tok.type = TOK_CONST_FLOAT;

#ifdef HAS_VECS 
		vector_fp_loop: if (src.size  - state->offset >= sizeof(NATIVE_VEC))
		{
			NATIVE_VEC x = *(NATIVE_VEC_PTR)(src.mem + state->offset);
			VEC_MASK end = (VEC_MASK)~RANGEMASK(x, '0', '9');
			if (__builtin_expect(end != 0, 1))
				state->offset += CTZ(end);
			else {
				state->offset += sizeof(NATIVE_VEC);
				goto vector_fp_loop;
			}
		}
#endif
		while (state->offset < src.size && xft_isdigit(src.mem[state->offset]))
			++state->offset;

		if (__builtin_expect(decimal_start == state->offset, 0)) goto float_follow_dot_error;

		t_u64a lhs = interpret_numeral(src.mem + real_part_start, real_part_end - real_part_start);
		t_f64a rhs = interpret_decimal_part(src.mem + decimal_start, state->offset - decimal_start);
		tok.as.value.f = (t_f64a)lhs + rhs;
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
			switch (src.mem[state->offset])
			{
				case '\\':
					state->offset += 2;
					goto strlit_loop;
				case '"':
					tok.as.lexeme.end = state->offset++;
					if (__builtin_expect(tok.as.lexeme.end - tok.as.lexeme.start > 64, 0)) goto strlit_too_big;
					goto success;
				default:
					++state->offset;
			}
		}
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
		if (__builtin_expect(state->offset < src.size && src.mem[state->offset] == dispatched, 1))
		{
			tok.type = dispatched;
			tok.as.lexeme.end = ++state->offset;
			goto success;
		}
		goto no_double_error;
	}
	div_or_comment:
	{
		if (state->offset < src.size && src.mem[state->offset] == dispatched)
		{
			t_any ptr = xft_memchr(src.mem + state->offset, '\n', src.size - state->offset);
			state->offset = (t_u32a)xft_tern(ptr != nullptr, (t_u32a)((t_uptr)ptr - (t_uptr)src.mem) + 1, src.size);
			goto whitespace;
		}
		tok.type = TOK_DIV;
		tok.as.lexeme.end = state->offset;
		goto success;
	}

	success:
		return (xft_soa_append_from_struct(allocator, soa, &tok, &token_soa_ctx));

#define LEX_ERR_FMT(msg) "error in %s:%u:%u: " msg "\n"
#define LEX_ERR(msg) xft_fatptr((t_any)LEX_ERR_FMT(msg), sizeof(LEX_ERR_FMT(msg)) - 1)

	no_double_error:
		fmt = LEX_ERR("&& and || have to appear as logical operators, no bit arithmetic is supported");
		goto report_error;
	error_unterminated_string:
		--tok.as.lexeme.start;
		fmt = LEX_ERR("unterminated string");
		goto report_error;
	ident_too_big:
		fmt = LEX_ERR("identifier too big");
		goto report_error;
	strlit_too_big:
		fmt = LEX_ERR("string literal too big");
		goto report_error;
	float_follow_dot_error:
		fmt = LEX_ERR("expected digit after '.' in float constant");
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
