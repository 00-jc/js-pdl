#pragma once

#include <xft.h>

typedef struct s_error_state
{
	bool		is_init;
	t_vec		line_offsets;
	t_writer	*__restrict__ writer;
}	t_error_state;

t_result	error_state_fill_lines(
	t_allocator							allocator,
	t_error_state	*__restrict__ const	state,
	const t_buffer						src
) __attribute__((__nonnull__(2)));

t_u32a		line_of_token(
	const t_error_state	*__restrict__ const	state,
	t_u32a									offset
) __attribute__((__nonnull__(1)));
