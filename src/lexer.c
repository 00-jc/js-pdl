#include <xft.h>
#include "diag.h"
#include "lexer.h"
#include "symbols.h"

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

__attribute__((noinline, __nonnull__(4, 5, 6), __hot__))
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
		['_']				= &&identifier,
		[0x80 ... UINT8_MAX]= &&identifier,
		['0' ... '9']		= &&numeric_constant,
		['"']				= &&string_literal,
	};
	DIAG_JUMP_TABLE_END


	#define KW_MAGIC 0x1fb2e0cfcf4095cbull
	#define KW_HASH(x) ((t_u32)(((x) * KW_MAGIC) >> 60))
	#define KW_TYPE_START 16
	// used: https://codeberg.org/00jc/repr (for the little endiand representation of the kws)
	//
	// mix the type table and the key tables,
	// since it's just identifiers a relative load
	// is cheaper and we can also bring it into L1
	// with less hassle.
	static const t_u64 kw_table[32] =
	{
		[0]  = 0x656C696877,         // while
		[1]  = 0x6F686365,           // echo
		[3]  = 0x74616F6C66,         // float
		[4]  = 0x6669,               // if
		[5]  = 0x65736C6166,         // false
		[6]  = 0x74706D6F7270,       // prompt
		[7]  = 0x6E61656C6F6F62,     // boolean
		[8]  = 0x64696F76,           // void
		[9]  = 0x6E7275746572,       // return
		[10] = 0x676E69727473,       // string
		[11] = 0x746E69,             // int
		[12] = 0x65757274,           // true
		[13] = 0x74656C,             // let
		[14] = 0x6E6F6974636E7566,   // function
		[15] = 0x6F64,               // do

		[KW_TYPE_START + 0]  = TOK_WHILE,
		[KW_TYPE_START + 1]  = TOK_ECHO,
		[KW_TYPE_START + 2]  = TOK_IDENTIFIER,
		[KW_TYPE_START + 3]  = TOK_FLOAT,
		[KW_TYPE_START + 4]  = TOK_IF,
		[KW_TYPE_START + 5]  = TOK_FALSE,
		[KW_TYPE_START + 6]  = TOK_PROMPT,
		[KW_TYPE_START + 7]  = TOK_BOOL,
		[KW_TYPE_START + 8]  = TOK_VOID,
		[KW_TYPE_START + 9]  = TOK_RETURN,
		[KW_TYPE_START + 10] = TOK_STRING,
		[KW_TYPE_START + 11] = TOK_INT,
		[KW_TYPE_START + 12] = TOK_TRUE,
		[KW_TYPE_START + 13] = TOK_LET,
		[KW_TYPE_START + 14] = TOK_FUNC,
		[KW_TYPE_START + 15] = TOK_DO,
	};


	t_symbol	sym;
	t_buffer	key;
	t_size		get_or_put;
	t_size		current_symbol;
	t_token		tok;
	t_u8		dispatched;
	t_buffer	fmt;
	t_u64		args[4];
	t_u32a		line;

whitespace:
	tok.type			= TOK_EOF;
	tok.as.value.i		= 0;
	tok.symbol			= 0;
whitespace_scalar:

	{	// @Jaime: benched: -10% rt on hotloop clang, -8% hotloop gcc. (100 us -> 90 us stable)
		//
		// min-non-prefetch: 80 us
		// min-prefetch: 50 us
		//
		// @Why: optimize for hot cases (do not pull &&unexpected labels into cache upfront)
		xft_prefetch_read0(states + 0x21, (0x3E - 0x21) * sizeof(t_any));	// ! , ; " 0123456789 <=>
		xft_prefetch_read0(states + 0x16, (0x7F - 0x61) * sizeof(t_any));	// a-z { | }
		xft_prefetch_read0(src.mem + state->offset, 64 + 8);				// 64 = max len for ids / strings + 8 for possible spaces
	}

	if (__builtin_expect(state->offset >= src.size, 0)) goto success;
	dispatched = src.mem[state->offset++];
	if (xft_isspace(dispatched)) goto whitespace_scalar;

	tok.as.lexeme.start = state->offset - 1;
	
	// @Jaime: lowercase for all letters, since we prefetched that case and this expression
	// is pure + branchless it can be pipelined properly, we can also remove the all &&identifier
	// labels from ['A' ... 'Z'], making less states.

	dispatched = (t_u8)xft_tern(dispatched - 'A' < 'Z' - 'A', dispatched | 0x20, dispatched);
	DIAG_GOTO(states[dispatched]);

	identifier: // [a-z][A-Z]_ | utf8
	{
		tok.type = TOK_IDENTIFIER;
		while (state->offset < src.size && (xft_isalpha(src.mem[state->offset]) | xft_isdigit(src.mem[state->offset]) | (src.mem[state->offset] == '_') | (src.mem[state->offset] >= 0x80)))
			++state->offset;
		if (__builtin_expect(64 < state->offset - tok.as.lexeme.start, 0)) goto ident_too_big;
		tok.as.lexeme.end = state->offset;
		t_u32a size = tok.as.lexeme.end - tok.as.lexeme.start;
		// (size >= 2 && size <= 8)
		if (size - 2 < 7)
		{
			t_u64a x = 0;
			t_u8 __attribute__	((uninitialized))	i[6];
			t_u8 __attribute__	((uninitialized))	s[6];

			xft_prefetch_read0(kw_table, (KW_TYPE_START + 15) * sizeof(t_u64));

			// variant of: https://codeberg.org/00jc/xft/src/branch/main/src/mem/arch/xft_memcpy.c
			//
			// LLVM nor gcc can't seem to inline the memcpy branch based on the size invariant
			// so i'll do it myself.
			//
			// invariant, first 2 bytes get always copied since size _has to_ be >= 2 to get in this
			// branch.
	
			i[0] = -(2ULL < size) & 2;
			i[1] = -(3ULL < size) & 3;
			i[2] = -(4ULL < size) & 4;
			i[3] = -(5ULL < size) & 5;
			i[4] = -(6ULL < size) & 6;
			i[5] = -(7ULL < size) & 7;
			s[0] = ((t_blk8r)src.mem + state->offset)[i[2]];
			s[1] = ((t_blk8r)src.mem + state->offset)[i[3]];
			s[2] = ((t_blk8r)src.mem + state->offset)[i[4]];
			s[3] = ((t_blk8r)src.mem + state->offset)[i[5]];
			s[4] = ((t_blk8r)src.mem + state->offset)[i[6]];
			s[5] = ((t_blk8r)src.mem + state->offset)[i[7]];
			x |= ((t_blk8r)src.mem + state->offset)[0];
			x |= ((t_blk8r)src.mem + state->offset)[1] << 8;
			x |= s[0] << (i[0] << 3);
			x |= s[1] << (i[1] << 3);
			x |= s[2] << (i[2] << 3);
			x |= s[3] << (i[3] << 3);
			x |= s[4] << (i[4] << 3);
			x |= s[5] << (i[5] << 3);

			// @Jaime: since the copies mightve repeated chars we mask it out
			// to make sure we don't mess up the identifier.

			x &= (1ULL << size) - 1;
			x = xft_from_le64(x);
			t_u32a hash = KW_HASH(x);
			tok.type = (t_token_type)xft_tern(kw_table[hash] == x, kw_table[KW_TYPE_START + hash], TOK_IDENTIFIER);
		}
		if (tok.type == TOK_IDENTIFIER)
		{
			current_symbol = state->symbol_dense_array->occupied_elements;
			key = xft_fatptr(src.mem + tok.as.lexeme.start, size);
			get_or_put = xft_indexmap_lookup(state->symbol_table, key);
			if (__builtin_expect(get_or_put == INDEXMAP_NPOS, 0))
			{
				sym = (t_symbol){
					.scope = 0,
				};
				if (__builtin_expect(xft_soa_append_from_struct(allocator, state->symbol_dense_array, &sym, &symbol_soa_ctx) == KO, 0))
					return KO;
				if (__builtin_expect(xft_indexmap_insert(allocator, state->symbol_table, key, current_symbol) == KO, 0))
					return KO;
				get_or_put = current_symbol;
			}
			tok.symbol = get_or_put;
		}
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
		tok.as.value.i = interpret_numeral(src.mem + tok.as.lexeme.start, state->offset - tok.as.lexeme.start);
		goto success;
	}

	float_constant:
	{
		t_u32a real_part_start = tok.as.lexeme.start;
		t_u32a real_part_end = state->offset - 1;
		t_u32a decimal_start = state->offset;
		tok.type = TOK_CONST_FLOAT;

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
		if (__builtin_expect(line_of_token_lazy_init(allocator, error_state, tok.as.lexeme.start, &line, src) == KO, 0))
			return (KO);
		args[0] = (t_u64)(t_uptr)file_name.mem;
		args[1] = file_name.size;
		args[2] = line;
		args[3] = tok.as.lexeme.start - ((const t_u32a *)error_state->line_offsets.buf.mem)[line - 1] + 1;
		xft_fmt_writer(error_state->writer, fmt, args);
		return (KO);
}
