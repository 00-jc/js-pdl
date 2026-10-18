#include <xft.h>
#include "diag.h"
#include "lexer.h"

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
__attribute__((__flatten__, __always_inline__, __nonnull__(1, 2), __hot__))
static inline void	ad_hoc_token_push(
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


	++soa->occupied_elements;
}

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

	#define EVEN			0xAAAAAAAAAAAAAAAA
	#define ODD				0x5555555555555555
	#define KW_MAGIC		0x1fb2e0cfcf4095cbull
	#define KW_HASH(x)		((t_u32)(((x) * KW_MAGIC) >> 60))
	#define KW_TYPE_START	16

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

	t_token		tok;
	t_buffer	fmt;
	t_u64		args[4];
	t_u32a		line;

	if (src.mem == nullptr) __builtin_unreachable();

	tok.type			= TOK_EOF;
	tok.as.value.i		= 0;

	t_nmask				newlines;
	t_nmask				whitespace;
	t_nmask				identifier;
	t_nmask				quotes;
	t_nmask				single_chars;
	t_nmask				pipe;
	t_nmask				amper;
	t_nmask				opeq;
	t_nmask				eq;
	t_nmask				div_comm;
	t_nmask				numconst;
	t_nmask				digits;
	t_nmask				not_recog;
	t_nmask				back_slashes;

	t_nmask				numeric_starts;
	t_nmask				non_printable;
	t_nmask				operator_starts;
	t_nmask				identifier_starts;
	t_nmask				single_char_starts;
	t_nmask				operator_followed_by_eq_starts;
	t_nmask				operator_followed_by_the_same_starts;
	t_nmask				starts;

	t_nmask				generic_mask;
	t_nvec				wide;

	constexpr t_size	MASK_SIZE_MIN1	= (sizeof(t_nmask) << 3) - 1;

	t_lexer_carry		next_carry;
	t_u8				next_byte;
	t_nmask				eq_next;
	t_nmask				amper_next;
	t_nmask				pipe_next;
	t_nmask				div_next;

	if (__builtin_expect(state->offset >= src.size, 0)) return (OK);
	if (__builtin_expect(state->starts == 0, 0))
	{

		// https://www.youtube.com/watch?v=E6dWIwPLx3s
		//
		// - carries stop the first position from being a start.

		t_u8 remain = (t_u8)(src.size - state->offset);
		generic_mask = (1ull << remain) - 1;
		wide = xft_vsplat(' ') & (xft_viota() >= xft_vsplat(remain));
		wide |= xft_vmask_load(src.mem + state->offset, generic_mask);

		t_u32a		next_byte_idx	= xft_tern(state->offset + sizeof(wide) < src.size, state->offset + sizeof(wide), state->offset);
		t_u32a		mask			= xft_tern(state->offset + sizeof(wide) < src.size, ~0, 0);

		next_byte	= src.mem[next_byte_idx] & mask;

		newlines		= xft_veqmask(wide, '\n');
		whitespace		= xft_vrangemask(wide, 0x09, 0x0B)	| xft_veqmask(wide, '\t')			| xft_veqmask(wide, '\r') | xft_veqmask(wide, ' ');
		identifier		= xft_vrangemask(wide, 'a', 'z')	| xft_vrangemask(wide, '0', '9')	| xft_veqmask(wide, '_');
		single_chars	= xft_veqmask(wide, ',')			| xft_veqmask(wide, ';')			| xft_veqmask(wide, '(')
						| xft_veqmask(wide, ')')			| xft_veqmask(wide, '{')			| xft_veqmask(wide, '}')
						| xft_veqmask(wide, '-')			| xft_veqmask(wide, '*')			| xft_veqmask(wide, '%')
						| xft_veqmask(wide, '+')			| xft_veqmask(wide, '=')			| xft_veqmask(wide, '/');
		digits			= xft_vrangemask(wide, '0', '9');
		numconst		= digits							| xft_veqmask(wide, '.');
		div_comm		= xft_veqmask(wide, '/');
		amper			= xft_veqmask(wide, '&');
		pipe			= xft_veqmask(wide, '|');
		quotes			= xft_veqmask(wide, '"');
		eq				= xft_veqmask(wide, '=');
		opeq			= xft_veqmask(wide, '<') | xft_veqmask(wide, '>') | xft_veqmask(wide, '+');
		non_printable	= ~(xft_vrangemask(wide, 0x20, 0x7E) | xft_vrangemask(wide, 0x80, 0xFF));
		back_slashes	= xft_veqmask(wide, '\\');

		// CLASSIFY(masks):

		eq_next									= (eq >> 1)			| ((t_nmask)(next_byte == '=') << MASK_SIZE_MIN1);
		amper_next								= (amper >> 1)		| ((t_nmask)(next_byte == '&') << MASK_SIZE_MIN1);
		pipe_next								= (pipe >> 1)		| ((t_nmask)(next_byte == '|') << MASK_SIZE_MIN1);
		div_next								= (div_comm >> 1)	| ((t_nmask)(next_byte == '/') << MASK_SIZE_MIN1);

		numeric_starts							= numconst & ~(((numconst | identifier) << 1) | (state->carry.ident_open | state->carry.num_open));
		numeric_starts							&= ~(numeric_starts << 1);

		identifier_starts						= (identifier & ~((identifier << 1) | state->carry.ident_open)) & ~digits; 

		single_char_starts						= single_chars | opeq;


		t_nmask		operator_followed_by_eq		= opeq & eq_next;
		t_nmask		opeq_seconds				= (operator_followed_by_eq << 1) | state->carry.opeq_pend;
		eq										&= ~opeq_seconds;
		single_char_starts						&= ~opeq_seconds;

		operator_followed_by_eq_starts			= operator_followed_by_eq;

		t_nmask		eqeq_firsts					= eq & ~(eq << 1) & ~(t_nmask)state->carry.eqeq_pend;
		t_nmask		eqeq_f_even					= (eqeq_firsts & (t_nmask)EVEN) | state->carry.eqeq_pend;
		t_nmask		eqeq_even_runs				= eq & ~(eq + eqeq_f_even);
		t_nmask		eqeq_odd_runs				= eq & ~eqeq_even_runs;
		t_nmask		eqeq_even_offset			= (eqeq_odd_runs & ODD) | (eqeq_even_runs & EVEN);

		t_nmask		and_firsts					= amper & ~(amper << 1) & ~(t_nmask)state->carry.and_pend;
		t_nmask		and_f_even					= (and_firsts & (t_nmask)EVEN) | state->carry.and_pend;
		t_nmask		and_even_runs				= amper & ~(amper + and_f_even);
		t_nmask		and_odd_runs				= amper & ~and_even_runs;
		t_nmask		and_even_offset				= (and_odd_runs & ODD) | (and_even_runs & EVEN);

		t_nmask		or_firsts					= pipe & ~(pipe << 1) & ~(t_nmask)state->carry.or_pend;
		t_nmask		or_f_even					= (or_firsts & (t_nmask)EVEN) | state->carry.or_pend;
		t_nmask		or_even_runs				= pipe & ~(pipe + or_f_even);
		t_nmask		or_odd_runs					= pipe & ~or_even_runs;
		t_nmask		or_even_offset				= (or_odd_runs & ODD) | (or_even_runs & EVEN);

		t_nmask		eqeq_pairs					= eqeq_even_offset & eq_next;
		t_nmask		and_pairs					= and_even_offset & amper_next;
		t_nmask		or_pairs					= or_even_offset & pipe_next;

		operator_followed_by_the_same_starts	= eqeq_pairs | and_pairs | or_pairs;

		t_nmask		same_seconds				= (operator_followed_by_the_same_starts << 1)
												| state->carry.eqeq_pend | state->carry.and_pend | state->carry.or_pend;

		single_char_starts						&= ~same_seconds;

		t_nmask		comment_start				= ((div_comm & div_next) | (t_nmask)state->carry.comment_open);
		t_nmask		comments					= ((newlines - comment_start) & ~newlines) | comment_start;
		t_u8		last_comment_mask_bit		= (comments >> MASK_SIZE_MIN1) & 1;

		next_carry.comment_open					= last_comment_mask_bit;
		single_chars							&= ~comment_start;

		t_nmask		escaped_n				= back_slashes & (xft_veqmask(wide, 'n') >> 1);
		t_nmask		escaped_t				= back_slashes & (xft_veqmask(wide, 't') >> 1);
		t_nmask		escaped_v				= back_slashes & (xft_veqmask(wide, 'v') >> 1);
		t_nmask		escaped_r				= back_slashes & (xft_veqmask(wide, 'r') >> 1);

		t_nmask		potential_escape		= back_slashes & ~state->carry.backsl_pend;
		t_nmask		code					= ((((potential_escape << 1) | EVEN) - potential_escape) ^ EVEN);
		t_nmask		escaped					= code ^ (back_slashes | state->carry.backsl_pend);
		t_nmask		real_quotes				= quotes & ~escaped;

		starts = operator_followed_by_eq_starts
				|operator_followed_by_the_same_starts
				|numeric_starts
				|identifier_starts
				|single_char_starts;

		starts &= ~comments;

		// TODO: string mask, the strings can have escapes, also then CANNOT have non-printable chars in them
		// for some reason.

		not_recog		= whitespace
						| identifier
						| numconst
						| single_chars
						| opeq
						| operator_followed_by_the_same_starts
						| same_seconds
						| comments;
		not_recog		= ~not_recog;

		next_carry.ident_open	= (identifier				>> MASK_SIZE_MIN1) & 1;
		next_carry.num_open		= (numconst					>> MASK_SIZE_MIN1) & 1;
		next_carry.opeq_pend	= (operator_followed_by_eq	>> MASK_SIZE_MIN1) & 1;
		next_carry.eqeq_pend	= (eqeq_pairs				>> MASK_SIZE_MIN1) & 1;
		next_carry.and_pend		= (and_pairs				>> MASK_SIZE_MIN1) & 1;
		next_carry.or_pend		= (or_pairs					>> MASK_SIZE_MIN1) & 1;
		next_carry.backsl_pend	= ((code & back_slashes)	>> MASK_SIZE_MIN1) & 1;
		next_carry.string_open	= state->carry.string_open;
		state->carry			= next_carry;

		if (__builtin_expect(not_recog != 0, 0))
		{
				state->offset += xft_memctz_u64(not_recog);
				tok.as.lexeme.start = state->offset;
				switch (src.mem[tok.as.lexeme.start])
				{
					case '&': __attribute__((__fallthrough__));
					case '|':
						goto no_double_error;
					default:
						goto unexpected_char;
				}
		}

		// intern all tokens at once, do NOT add the count to soa->occupied_elements.

		// detect kws, match lengths (simd too).
		
		// advance state->offset by the lane size, token continuations are
		// handled by state->carry.
	}
	else
	{
		// fake tokenization (simulates a next_token call)

		state->starts &= state->starts - 1;
		state->relative = xft_memctz_u64(state->starts);
		state->ends &= (1ULL << state->relative) - 1;
		++soa->occupied_elements;
	}
	return (OK);

#define LEX_ERR_FMT(msg) "error in %s:%u:%u: " msg "\n"
#define LEX_ERR(msg) xft_fatptr((t_any)LEX_ERR_FMT(msg), sizeof(LEX_ERR_FMT(msg)) - 1)

	no_double_error:
		fmt = LEX_ERR("& and | have to appear as logical operators, no bit arithmetic is supported");
		goto report_error;
// 	error_unterminated_string:
// 		--tok.as.lexeme.start;
// 		fmt = LEX_ERR("unterminated string");
// 		goto report_error;
// 	ident_too_big:
// 		fmt = LEX_ERR("identifier too big");
// 		goto report_error;
// 	strlit_too_big:
// 		fmt = LEX_ERR("string literal too big");
// 		goto report_error;
// 	float_follow_dot_error:
// 		fmt = LEX_ERR("expected digit after '.' in float constant");
// 		goto report_error;
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
