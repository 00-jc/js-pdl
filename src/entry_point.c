#include <xft.h>

__attribute__((__nonnull__(1), noreturn))
void	xft_main(const t_any *__restrict__ const sp)
{
	// @Jaime: roadmap:
	//
	// - use xft_map_file to get the file contents
	// - set up the tokenizer loop
	// - tokenize it all (call next_token() -> Token)
	// - open the dump file
	// - use xft_fmt_writer + xft_get_fs_writer to write and flush to the file
	//
	//
	// @Speed:
	//
	// - For memory layout we're doing a Vec<SoA<Token>> for the tokens,
	// these being:
	//
	// typedef struct s_token {
	//		type_of_token_t		type;
	//		t_u32a				start;
	// }	t_token; (token is not defined yet)
	//
	// we can reconstruct the lexeme later.
	//
	// in case we need to parse the numerals and directly hold
	// their values, we'll have a vec of constants and the start
	// will hold an absolute reference to that.
	//
	// the ownership of this vector goes in the stack-space
	// of the parser struct, or whatever calls next_token eventually.
	//
	// - As per the tokenizer algo, i'd like to use simd:
	//		- t_blkXXX and t_vuXXX are available through xft.
	//
	//	 if that's not feasible, we'll use computed gotos
	//	 AND simd optimizations once we are in a dispatched
	//	 state that supports it.

	(void)sp; // @Jaime: make it compile with -Wall
	xft_exit(0);
}
