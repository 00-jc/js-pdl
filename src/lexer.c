#include <xft.h>
#include "diag.h"
#include "lexer.h"
#include "perfect_hash.h"

DIAG_FAM_INIT_BEGIN
const t_soa_context	token_soa_ctx = {
	.total_struct_size	= TOKEN_SOA_TYPE_SIZE + TOKEN_SOA_AS_SIZE,
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
	},
};
DIAG_FAM_INIT_END

// @Jaime: neglible win over xft's generic soa (x0.10), gcc nor clang unroll it
// apparently
__attribute__((__flatten__, __always_inline__, __nonnull__(1, 2)))
static inline void	pdl__ad_hoc_token_push(
	t_soa *__restrict__ const soa,
	t_any __restrict__ const _struct)
{
	const t_size	n = soa->occupied_elements;
	const t_size	cap = soa->capacity_elements;
	t_u8 *const		base = soa->ptr;
	t_size			off;
	t_size			size;
	t_any			start;

	if (base == nullptr) __builtin_unreachable();
	size = token_soa_ctx.entries[0].size;
	off = token_soa_ctx.entries[0].offset_from_struct_base;
	start = (t_any)(base + cap * token_soa_ctx.entries[0].size_of_prev_fields
			+ n * size);
	xft_memcpy(start, (t_blk8r)_struct + off, size);
	size = token_soa_ctx.entries[1].size;
	off = token_soa_ctx.entries[1].offset_from_struct_base;
	start = (t_any)(base + cap * token_soa_ctx.entries[1].size_of_prev_fields
			+ n * size);
	xft_memcpy(start, (t_blk8r)_struct + off, size);
	soa->occupied_elements = n + 1;
}

__attribute__((__nonnull__(1), pure, __always_inline__, __unused__))
static inline t_u64a		pdl__interpret_numeral(const t_u8 *__restrict__ const mem, t_u32a len)
{
	t_u64a	value;
	t_size	it;

	value = 0;
	it = 0;
	while (it < len) value = (value << 3) + (value << 1) + (mem[it++] - '0');
	return value;
}

__attribute__((__nonnull__(1), pure, __always_inline__, __unused__))
static inline t_f64a		pdl__interpret_decimal_part(const t_u8 *__restrict__ const mem, t_u32a len)
{
	t_f64a	value;
	t_size	it;

	value = 0;
	it = len;
	while (it > 0) value = (value + (mem[--it] - '0')) / 10;
	return value;
}

__attribute__((noinline, __nonnull__(4, 5, 6), __hot__))
t_result	pdl_lex_next_token(
	t_allocator							allocator,
	const t_buffer						src,
	const t_buffer						file_name,
	t_lexer_state	*__restrict__ const	state,
	t_soa			*__restrict__ const	soa,
	t_error_state	*__restrict__ const	error_state
)
{
	t_token		tok;
	t_u8		dispatched;
	t_buffer	fmt;
	t_u64		args[4];
	t_u32a		line;

	t_nvec		wide;
	t_nmask		mask;
	t_nmask		valid;
	t_size		rem;
	t_size		lane;
	t_u32a		point;

	if (src.mem == nullptr) __builtin_unreachable();

whitespace:
	tok.type			= TOK_EOF;
	tok.as.value.i		= 0;

	// 64 = max len for ids / strings + 8 for possible spaces
	xft_prefetch_read0((t_cany)(src.mem + state->offset), 64 + 8);

	do
	{
		rem = src.size - state->offset;
		valid = xft_tern(rem < sizeof(wide), (t_nmask)((1ull << rem) - 1), (t_nmask)~0);
		wide = xft_vmask_load(src.mem + state->offset, valid);
		mask = ~(xft_vrangemask(wide, 9, 13) | xft_veqmask(wide, ' '));
		lane = xft_memctz_u64(mask);
		state->offset += xft_tern(mask != 0, lane, sizeof(wide));
	} while (mask == 0);

	if (__builtin_expect(state->offset >= src.size, 0)) goto success;
	dispatched = src.mem[state->offset++];

	tok.as.lexeme.start = state->offset - 1;
	
	switch (dispatched)
	{
		case 0:
			goto success;
		case ',': __attribute__((__fallthrough__));
		case ';': __attribute__((__fallthrough__));
		case '(': __attribute__((__fallthrough__));
		case ')': __attribute__((__fallthrough__));
		case '{': __attribute__((__fallthrough__));
		case '}': __attribute__((__fallthrough__));
		case '-': __attribute__((__fallthrough__));
		case '*': __attribute__((__fallthrough__));
		case '%':
			goto single_char_token;
		case '|': __attribute__((__fallthrough__));
		case '&':
			goto double_op_or_error;
		case '=': __attribute__((__fallthrough__));
		case '!': __attribute__((__fallthrough__));
		case '<': __attribute__((__fallthrough__));
		case '>': __attribute__((__fallthrough__));
		case '+':
			goto possible_operator_followed_by_eq;
		case '/':
			goto div_or_comment;
		case '0': __attribute__((__fallthrough__));
		case '1': __attribute__((__fallthrough__));
		case '2': __attribute__((__fallthrough__));
		case '3': __attribute__((__fallthrough__));
		case '4': __attribute__((__fallthrough__));
		case '5': __attribute__((__fallthrough__));
		case '6': __attribute__((__fallthrough__));
		case '7': __attribute__((__fallthrough__));
		case '8': __attribute__((__fallthrough__));
		case '9':
			goto numeric_constant;
		case '"':
			goto string_literal;
		default:
			point = xft_xid_load_point(src.mem + tok.as.lexeme.start, src.size - tok.as.lexeme.start);
			if (!xft_xid_is_start(xft_xid_decode(point))) goto unexpected_char;
			goto identifier;
	}

	identifier: // [a-zA-Z_]
	{
		tok.type = TOK_IDENTIFIER;

		// [a-zA-Z_][a-zA-Z0-9_]+
		mask = ~(xft_vrangemask(wide | 0x20, 'a', 'z') | xft_vrangemask(wide, '0', '9') | xft_veqmask(wide, '_')) >> lane;
		state->offset = (t_u32a)(tok.as.lexeme.start + xft_tern(mask != 0, xft_memctz_u64(mask), sizeof(wide) - lane)); // leer
		while (mask == 0 && (rem = src.size - state->offset) != 0)
		{
			valid = xft_tern(rem < sizeof(wide), (t_nmask)((1ull << rem) - 1), (t_nmask)~0);
			wide = xft_vmask_load(src.mem + state->offset, valid);
			mask = ~(xft_vrangemask(wide | 0x20, 'a', 'z') | xft_vrangemask(wide, '0', '9') | xft_veqmask(wide, '_'));
			state->offset += xft_tern(mask != 0, xft_memctz_u64(mask), sizeof(wide)); // leer
		}

		// utf8 and whatever the vector path stopped on
		while (state->offset < src.size)
		{
			point = xft_xid_load_point(src.mem + state->offset, src.size - state->offset);
			if (!xft_xid_is_continue(xft_xid_decode(point))) break;
			state->offset += xft_xid_codepoint_len(point);
		}

		if (__builtin_expect(64 < state->offset - tok.as.lexeme.start, 0)) goto ident_too_big;
		tok.as.lexeme.end = state->offset;
		t_u32a size = tok.as.lexeme.end - tok.as.lexeme.start;
		if (size - 2 < 7) // (size >= 2 && size <= 8)
		{
			xft_prefetch_read0(TABLE_REPR, (KW_TYPE_START + REPRESENTATIVE_TABLE_SIZE) * sizeof(t_u64));

			// LLVM nor gcc can't seem to inline the memcpy branch based on the size invariant
			// so i'll do a overlap load myself.
			//
			// invariant, first 2 bytes get always copied since size _has to_ be >= 2 to get in this
			// branch.

			t_u64 x = 0;
			t_u64 z = 0;

			const t_blk8r	ptr		= src.mem + tok.as.lexeme.start;
			const t_blk8r	base	= (t_any)xft_tern(4 < size, (t_uptr)src.mem + tok.as.lexeme.start, (t_uptr)&z);
			const t_u8		offset	= (t_u8)xft_tern(4 < size, size - 4, 0); 

			if (base == nullptr) __builtin_unreachable();

			x |= *(const t_u32 *__restrict__ const)base;
			x |= *(const t_u16 *__restrict__ const)ptr;
			x |= (t_u64)(*(const t_u16 *__restrict__ const)(ptr + size - 2)) << ((size - 2) << 3);
			x |= (t_u64)(*(const t_u16 *__restrict__ const)(base + offset)) << (offset << 3);

			x = xft_from_le64(x);
			t_u32a hash = (t_u32a)representative_bits_hash(x);
			tok.type = (t_token_type)xft_tern(TABLE_REPR[hash] == x, TABLE_REPR[KW_TYPE_START + hash], TOK_IDENTIFIER);
		}
		goto success; // Generar token
	}
	
	numeric_constant: // 0-9
	{
		tok.type = TOK_CONST_INT;

		while (state->offset < src.size && xft_isdigit(src.mem[state->offset])) //[0-9]+
			++state->offset; // leer
		if (state->offset < src.size && src.mem[state->offset] == '.') // [0-9]+.
		{
			++state->offset; // leer
			goto float_constant;
		}
		tok.as.value.i = pdl__interpret_numeral(src.mem + tok.as.lexeme.start, state->offset - tok.as.lexeme.start); // obtener valor
		goto success; // Generar Token
	}

	float_constant: // [0-9]+.
	{
		t_u32a real_part_start = tok.as.lexeme.start;
		t_u32a real_part_end = state->offset - 1;
		t_u32a decimal_start = state->offset;
		tok.type = TOK_CONST_FLOAT;

		while (state->offset < src.size && xft_isdigit(src.mem[state->offset])) // [0-9]+.[0-9]+
			++state->offset;

		if (__builtin_expect(decimal_start == state->offset, 0)) goto float_follow_dot_error;

		t_u64a lhs = pdl__interpret_numeral(src.mem + real_part_start, real_part_end - real_part_start); // obtener valor entero
		t_f64a rhs = pdl__interpret_decimal_part(src.mem + decimal_start, state->offset - decimal_start); // obtener valor decimal
		tok.as.value.f = (t_f64a)lhs + rhs; // obtener valor
		goto success; // Generar token
	}

	string_literal: // "
	{
		t_u8	compound_elements = 0;

		tok.type = TOK_STRING_LITERAL;
		tok.as.lexeme.start = state->offset;
		while (1)
		{
			strlit_loop:  // "c*
			if (__builtin_expect(state->offset >= src.size, 0)) goto error_unterminated_string;
			switch (src.mem[state->offset])
			{
DIAG_CASE_RANGE_BEGIN
				case 0x00 ... 0x1F: // incompatibles
DIAG_CASE_RANGE_END
					goto error_string_cannot_contain_unprintable;
				case '\\':	// "c*\ escape char 
					t_u32a idx		= (t_u32a)xft_tern(state->offset >= src.size, state->offset, state->offset + 1);
					t_u32a mask_ 	= (t_u32a)xft_tern(state->offset >= src.size, 0, ~0);
					t_u8 c			= src.mem[idx] & mask_;
					t_u8 is_escaped	= !!(	(c == '\\')
										|	(c == 'n')
										|	(c == 't')
										|	(c == 'r')
										|	(c == '"')
										|	(c == 'v'));

					compound_elements	+= is_escaped;
					state->offset		+= xft_tern(is_escaped, 2, 1); // leer
					goto strlit_loop;
				case '"': // "c*"
					tok.as.lexeme.end = state->offset++; // leer
					if (__builtin_expect((tok.as.lexeme.end - tok.as.lexeme.start) - compound_elements > 64, 0)) goto strlit_too_big;
					goto success; // Generar token
DIAG_CASE_RANGE_BEGIN
				case 0x80 ... 0xFF: __attribute__((__fallthrough__));
DIAG_CASE_RANGE_END
				default:
					++state->offset; // leer
			}
		}
	}

	possible_operator_followed_by_eq: // + | < | > | ! 
	{
		t_u64a			cond		= state->offset < src.size;
		t_u32a			idx	 		= (t_u32a)xft_tern(cond, state->offset, state->offset - 1);
		t_u32a			next 		= (t_u32a)xft_tern(cond, src.mem[idx], 0);
		t_token_type 	type 		= (t_token_type)xft_tern(next == '=', dispatched | 0x80, dispatched);
		t_u64a			newOffset	= xft_tern(next == '=', 1, 0);

		state->offset += newOffset; // leer
		tok.as.lexeme.end = state->offset;
		tok.type = type;
		goto success; // Generar token
	}
	single_char_token: // , | ; | ( | ) | { | } | - | * | %
	{
		tok.type			= dispatched;
		tok.as.lexeme.end	= state->offset;
		goto success; // Generar token
	}
	double_op_or_error: // | | &
	{
		if (__builtin_expect(state->offset < src.size && src.mem[state->offset] == dispatched, 1)) // || | &&
		{
			tok.type = dispatched;
			tok.as.lexeme.end = ++state->offset; // leer
			goto success; // Generar token
		}
		goto no_double_error; // Error
	}
	div_or_comment: // /
	{
		if (__builtin_expect(state->offset < src.size && src.mem[state->offset] == dispatched, 0)) // //
		{
			t_any ptr = xft_memchr(src.mem + state->offset, '\n', src.size - state->offset); // //c*\n
			state->offset = (t_u32a)xft_tern(ptr != nullptr, (t_u32a)((t_uptr)ptr - (t_uptr)src.mem) + 1, src.size); // leer
			goto whitespace; // Buscar el verdadero token, saltar comentario
		}
		tok.type = TOK_DIV;
		tok.as.lexeme.end = state->offset;
		goto success; // Generar token
	}

	success: // Generación de tokens
		if (__builtin_expect(soa->occupied_elements == soa->capacity_elements, 0)
			&& !xft_soa_reserve(allocator, soa, &token_soa_ctx, 1))
			return (KO);
		pdl__ad_hoc_token_push(soa, &tok);
		return (OK);

// ERRORES
#define LEX_ERR_FMT(msg) "error in %s:%u:%u: " msg "\n"
#define LEX_ERR(msg) xft_fatptr((t_any)LEX_ERR_FMT(msg), sizeof(LEX_ERR_FMT(msg)) - 1)

	no_double_error:
		fmt = LEX_ERR("&& and || have to appear as logical operators, no bit arithmetic is supported");
		goto report_error;
	error_string_cannot_contain_unprintable:
		--tok.as.lexeme.start;
		fmt = LEX_ERR("strings cannot contain unprintable chars");
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
		if (__builtin_expect(pdl_line_of_token_lazy_init(allocator, error_state, tok.as.lexeme.start, &line, src) == KO, 0))
			return (KO);
		args[0] = (t_u64)(t_uptr)file_name.mem;
		args[1] = file_name.size;
		args[2] = line;
		args[3] = tok.as.lexeme.start - ((const t_u32a *)error_state->line_offsets.buf.mem)[line - 1] + 1;
		xft_fmt_writer(error_state->writer, fmt, args);
		return (KO);
}
