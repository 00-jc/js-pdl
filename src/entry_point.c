#include <xft.h>
#include "js-pdl.h"

__attribute__((__nonnull__(1), noreturn))
void	xft_main(const t_any *__restrict__ const sp)
{
	t_kernel_ptrs	ptrs;
	t_writer		_stdout;
	t_writer		_stderr;
	t_u8			_stdout_buffer[1024];
	t_u8			_stderr_buffer[1024];
	t_error_state	err_state;
	t_gpa			gpa;
	t_allocator		allocator;
	t_buffer		src_buf;
	t_soa			tokens_soa;
	t_lexer_state	lexer_state;
	t_buffer		filename;
	t_result		result;

	ptrs = xft_get_kernel_ptrs(sp);
	_stderr = xft_get_fs_writer(xft_fatptr(_stderr_buffer, 1024), xft_get_stderr());
	_stdout = xft_get_fs_writer(xft_fatptr(_stdout_buffer, 1024), xft_get_stdout());

	if (__builtin_expect(ptrs.argc != 2, 0))
	{
		xft_writer_write(&_stderr, xft_fatptr((t_any)USAGE, sizeof(USAGE) - 1));
		xft_writer_flush(&_stderr);
		xft_exit(1);
	}

	gpa = xft_new_gpa();
	if (__builtin_expect(gpa.slab == nullptr, 0))
	{
		xft_writer_write(&_stderr, xft_fatptr((t_any)ALLOC_INIT_OOM, sizeof(ALLOC_INIT_OOM) - 1));
		xft_writer_flush(&_stderr);
		xft_exit(1);
	}

	allocator = xft_gpa_allocator(&gpa);

	src_buf = xft_map_file(ptrs.argv[1], O_RDONLY);
	if (__builtin_expect(src_buf.mem == nullptr, 0))
	{
		t_stat	st;

		// mmap of a 0-byte file fails, but an empty program is valid
		if (xft_stat((t_any)ptrs.argv[1], &st) == 0 && st.st_size == 0)
		{
			xft_gpa_destroy(&gpa);
			xft_exit(0);
		}
		xft_writer_write(&_stderr, xft_fatptr((t_any)CANNOT_MAP_FILE, sizeof(CANNOT_MAP_FILE) - 1));
		xft_gpa_destroy(&gpa);
		xft_writer_flush(&_stderr);
		xft_exit(1);
	}

	tokens_soa = xft_new_soa(allocator, 512, &token_soa_ctx);
	if (__builtin_expect(tokens_soa.ptr == nullptr, 0))
	{
		xft_writer_write(&_stderr, xft_fatptr((t_any)TOKENS_SOA_OOM, sizeof(TOKENS_SOA_OOM) - 1));
		xft_gpa_destroy(&gpa);
		xft_unmap_file(src_buf);
		xft_writer_flush(&_stderr);
		xft_exit(1);
	}

	err_state.line_offsets = xft_new_vec(allocator, 64, sizeof(t_u32a));
	if (__builtin_expect(err_state.line_offsets.buf.mem == nullptr
		|| error_state_fill_lines(allocator, &err_state, src_buf) == KO, 0))
	{
		xft_writer_write(&_stderr, xft_fatptr((t_any)LINES_OOM, sizeof(LINES_OOM) - 1));
		xft_destroy_soa(allocator, &tokens_soa);
		xft_gpa_destroy(&gpa);
		xft_unmap_file(src_buf);
		xft_writer_flush(&_stderr);
		xft_exit(1);
	}
	lexer_state = (t_lexer_state)
	{
		.offset = 0,
	};
	filename = xft_fatptr(ptrs.argv[1], xft_strlen((t_any)ptrs.argv[1]));
	err_state.writer = &_stderr;
	while ((result = lex_next_token(allocator, src_buf, filename, &lexer_state, &tokens_soa, &err_state)) == OK)
	{
		const t_token_type *__restrict__ const ttype = xft_soa_fields(&tokens_soa, &token_soa_ctx, 0);
		if (__builtin_expect(ttype[tokens_soa.occupied_elements - 1] == TOK_EOF, 0)) break;
	}

	xft_unmap_file(src_buf);
	xft_vec_destroy(allocator, &err_state.line_offsets);
	xft_destroy_soa(allocator, &tokens_soa);
	xft_gpa_destroy(&gpa);
	xft_writer_flush(&_stdout);
	xft_writer_flush(&_stderr);
	xft_exit(result == KO);
}
