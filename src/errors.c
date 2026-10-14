#include "errors.h"

t_result	error_state_fill_lines(t_allocator allocator, t_error_state *__restrict__ const state, const t_buffer src)
{
	t_u32a		scan;
	t_u32a		diff;
	t_u8		*ptr;

	if (src.mem == nullptr) __builtin_unreachable();
	scan = 0;
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

__attribute__((__nonnull__((1))))
t_u32a	line_of_token(const t_error_state *__restrict__ const state, t_u32a offset)
{
	const t_u32a	*arr;
	const t_u32a	*base;
	t_u32a			n;
	t_u32a			half;

	arr = (const t_u32a *)state->line_offsets.buf.mem;
	n = (t_u32a)xft_vec_len(&state->line_offsets, sizeof(t_u32a));
	if (__builtin_expect(n == 0, 0)) return (0);
	base = arr;
	while (n > 1)
	{
		half = n >> 1;
		base = (t_any)xft_tern(base[half] <= offset, (t_uptr)(base + half), (t_uptr)base);
		n -= half;
	}
	return ((t_u32a)(base - arr)) + 1;
}
