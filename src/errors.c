#include "errors.h"

__attribute__((__cold__, __nonnull__(2), __flatten__, __unused__, __always_inline__))
inline t_result	pdl__lazy_error_state_fill_lines(t_allocator allocator, t_error_state *__restrict__ const state, const t_buffer src)
{
	t_u32a		scan;
	t_u32a		diff;
	t_u8		*ptr;

	if (src.mem == nullptr) __builtin_unreachable();
	scan = 0;
	state->line_offsets = xft_new_vec(allocator, 128, sizeof(t_u32a));
	if (__builtin_expect(state->line_offsets.buf.mem == nullptr, 0))
		return (KO);
	state->is_init = true;
	if (__builtin_expect(xft_vec_push_back(allocator, &state->line_offsets, (t_blk8r)&scan, sizeof(t_u32a)) == KO, 0))
		return (KO);
	while ((ptr = xft_memchr(src.mem + scan, '\n', src.size - scan)))
	{
		diff = (t_u32a)((t_uptr)ptr - (t_uptr)src.mem) + 1;
		if (__builtin_expect(xft_vec_push_back(allocator, &state->line_offsets, (t_blk8r)&diff, sizeof(t_u32a)) == KO, 0))
			return (KO);
		scan = diff;
	}
	return (OK);
}

__attribute__((__nonnull__(2, 4), __cold__, noinline))
t_result	pdl_line_of_token_lazy_init(
		t_allocator								allocator,
		t_error_state *__restrict__ const		state,
		t_u32a									offset,
		t_u32a *__restrict__ const				line_of_token,
		t_buffer								src
)
{
	const t_u32a	*arr;
	const t_u32a	*base;
	t_u32a			n;
	t_u32a			half;

	if (__builtin_expect(state->is_init == false, 0))
	{
		if (__builtin_expect(pdl__lazy_error_state_fill_lines(allocator, state, src) == KO, 0))
			return (KO);
	}
	arr = (const t_u32a *)state->line_offsets.buf.mem;
	n = (t_u32a)xft_vec_len(&state->line_offsets, sizeof(t_u32a));
	if (__builtin_expect(n == 0, 0))
	{
		*line_of_token = 0;
		return (OK);
	}
	base = arr;
	while (n > 1)
	{
		half = n >> 1;
		base = (t_any)xft_tern(base[half] <= offset, (t_uptr)(base + half), (t_uptr)base);
		n -= half;
	}
	*line_of_token = ((t_u32a)(base - arr)) + 1;
	return (OK);
}
