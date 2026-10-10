#include <xft.h>

#define USAGE "usage: <exe> <file.js>\n"

__attribute__((__nonnull__(1), noreturn))
void	xft_main(const t_any *__restrict__ const sp)
{
	t_kernel_ptrs	ptrs;
	t_writer		_stdout;
	t_writer		_stderr;
	t_u8			_stdout_buffer[1024];
	t_u8			_stderr_buffer[1024];

	ptrs = xft_get_kernel_ptrs(sp);
	_stderr = xft_get_fs_writer(xft_fatptr(_stderr_buffer, 1024), xft_get_stderr());
	_stdout = xft_get_fs_writer(xft_fatptr(_stdout_buffer, 1024), xft_get_stdout());

	if (__builtin_expect(ptrs.argc != 2, 0))
	{
		xft_writer_write(&_stderr, xft_fatptr((t_any)USAGE, sizeof(USAGE)));
		xft_writer_flush(&_stderr);
		xft_exit(1);
	}

	xft_writer_flush(&_stdout);
	xft_writer_flush(&_stderr);
	xft_exit(0);
}
