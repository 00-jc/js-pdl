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
	t_soa			symbols_soa;
	t_lexer_state	lexer_state;
	t_buffer		filename;
	t_result		result;
	t_indexmap		symbols;

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

	symbols_soa = xft_new_soa(allocator, 1024, &symbol_soa_ctx);
	if (__builtin_expect(symbols_soa.ptr == nullptr, 0))
	{
		xft_writer_write(&_stderr, xft_fatptr((t_any)TOKENS_SOA_OOM, sizeof(TOKENS_SOA_OOM) - 1));
		xft_destroy_soa(allocator, &tokens_soa);
		xft_gpa_destroy(&gpa);
		xft_unmap_file(src_buf);
		xft_writer_flush(&_stderr);
		xft_exit(1);
	}

	symbols = xft_new_indexmap_with(allocator, 1024);
	if (__builtin_expect(symbols.soa.ptr == nullptr, 0))
	{
		xft_writer_write(&_stderr, xft_fatptr((t_any)LINES_OOM, sizeof(LINES_OOM) - 1));
		xft_destroy_soa(allocator, &tokens_soa);
		xft_destroy_soa(allocator, &symbols_soa);
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
		xft_destroy_soa(allocator, &symbols_soa);
		xft_indexmap_destroy(allocator, &symbols);
		xft_gpa_destroy(&gpa);
		xft_unmap_file(src_buf);
		xft_writer_flush(&_stderr);
		xft_exit(1);
	}
	lexer_state = (t_lexer_state)
	{
		.offset = 0,
		.symbol_dense_array = &symbols_soa,
		.symbol_table = &symbols,
	};
	filename = xft_fatptr(ptrs.argv[1], xft_strlen((t_any)ptrs.argv[1]));
	err_state.writer = &_stderr;
	while ((result = lex_next_token(allocator, src_buf, filename, &lexer_state, &tokens_soa, &err_state)) == OK)
	{
		const t_token_type *__restrict__ const ttype = xft_soa_fields(&tokens_soa, &token_soa_ctx, 0);
		if (__builtin_expect(ttype[tokens_soa.occupied_elements - 1] == TOK_EOF, 0)) break;
	}

	// <TIPO, ''|valor|tabla?)>\n

	// @Jaime: for debugging
	if (result == OK)
	{
		const t_token_type *__restrict__ const ttype = xft_soa_fields(&tokens_soa, &token_soa_ctx, TOKEN_SOA_TYPE);
		const t_token_body_union *__restrict__ const tvalues = xft_soa_fields(&tokens_soa, &token_soa_ctx, TOKEN_SOA_AS);
		const t_uptr *__restrict__ const tsymbols = xft_soa_fields(&tokens_soa, &token_soa_ctx, TOKEN_SOA_SYMBOL);
		for (t_size i = 0; i < tokens_soa.occupied_elements; i++)
		{
			t_buffer	str;
			t_u64a		args[4];

			str = token_type_to_string(ttype[i]);
			args[0] = (t_uptr)str.mem;
			args[1] = str.size;
			switch (ttype[i])
			{
				case TOK_IDENTIFIER:
					args[2] = tsymbols[i];
					xft_fmt_writer(&_stdout, xft_fatptr((t_any)"<%s, %q>\n", sizeof("<%s, %q>\n") - 1), (t_any)&args);
					break;
				case TOK_STRING_LITERAL:
					args[2] = (t_uptr)src_buf.mem + tvalues[i].lexeme.start;
					args[3] = (t_u64a)(tvalues[i].lexeme.end - tvalues[i].lexeme.start);
					xft_fmt_writer(&_stdout, xft_fatptr((t_any)"<%s, \"%s\">\n", sizeof("<%s, \"%s\">\n") - 1), (t_any)&args);
					break;
				case TOK_CONST_INT:
					args[2] = tvalues[i].value.i;
					xft_fmt_writer(&_stdout, xft_fatptr((t_any)"<%s, %q>\n", sizeof("<%s, %q>\n") - 1), (t_any)&args);
					break;
				case TOK_CONST_FLOAT:
					args[2] = tvalues[i].value.i;
					xft_fmt_writer(&_stdout, xft_fatptr((t_any)"<%s, %f>\n", sizeof("<%s, %f>\n") - 1), (t_any)&args);
					break;
				default:
					xft_fmt_writer(&_stdout, xft_fatptr((t_any)"<%s,>\n", sizeof("<%s,>\n") - 1), (t_any)&args);
			}
		}
	}

	xft_unmap_file(src_buf);
	xft_vec_destroy(allocator, &err_state.line_offsets);
	xft_destroy_soa(allocator, &tokens_soa);
	xft_destroy_soa(allocator, &symbols_soa);
	xft_indexmap_destroy(allocator, &symbols);
	xft_gpa_destroy(&gpa);
	xft_writer_flush(&_stdout);
	xft_writer_flush(&_stderr);
	xft_exit(result == KO);
}
