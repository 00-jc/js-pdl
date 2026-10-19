#pragma once

#include <xft.h>

typedef struct s_error_state
{
	bool		is_init;
	t_vec		line_offsets;
	t_writer	*__restrict__ writer;
}	t_error_state;

t_result	pdl_line_of_token_lazy_init(
	t_allocator							allocator,
	t_error_state	*__restrict__ const	state,
	t_u32a								offset,
	t_u32a			*__restrict__ const	line_of_token,
	t_buffer							src
) __attribute__((__nonnull__(2, 4)));
