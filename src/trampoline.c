#include <xft.h>

void	xft_main_v1(const t_any *__restrict__ const sp) __attribute__((__nonnull__(1), noreturn));
void	xft_main_v2(const t_any *__restrict__ const sp) __attribute__((__nonnull__(1), noreturn));
void	xft_main_v3(const t_any *__restrict__ const sp) __attribute__((__nonnull__(1), noreturn));
void	xft_main_v4(const t_any *__restrict__ const sp) __attribute__((__nonnull__(1), noreturn));

__attribute__((__nonnull__(1), noreturn))
void	xft_main(const t_any *__restrict__ const sp)
{
	switch (xft_dyn_cpu_select().x86_64_ver)
	{
		case XFT_X86_V4:	xft_main_v4(sp);
		case XFT_X86_V3:	xft_main_v3(sp);
		case XFT_X86_V2:	xft_main_v2(sp);
		default:			xft_main_v1(sp);
	}
}
