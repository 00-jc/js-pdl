#include <xft.h>
#include "js-pdl.h"
#include "perfect_hash_cli.h"

static constexpr char	ERR_USAGE[] = "usage: <exe> <file.js> [--tokens <out>] [--symbols <out>] [--ast <out>]\n";
static constexpr char	ERR_MULTIPLE_SOURCES[] = "ERROR: multiple source files listed, run --help for cli interface\n";
static constexpr char	ERR_MISSING_FLAG_VALUE[] = "ERROR: flag is missing its output file\n";
static constexpr char	ERR_ALLOC_INIT[] = "ERROR: Cannot initialize allocator, most likely OOM\n";
static constexpr char	ERR_CANNOT_MAP_FILE[] = "ERROR: Cannot map source file, check that it exists and is readable\n";
static constexpr char	ERR_TOKENS_SOA_OOM[] = "ERROR: Cannot allocate token storage, most likely OOM\n";
static constexpr char	ERR_SYMBOLS_OOM[] = "ERROR: Cannot allocate symbol table, most likely OOM\n";
static constexpr char	ERR_CANNOT_OPEN_OUTFILE[] = "ERROR: Cannot open output file for writing\n";

typedef enum e_cli_flag
{
	FLAG_HELP,
	FLAG_AST,
	FLAG_TOKENS,
	FLAG_SYMBOLS,
	FLAG_NONE,
}	t_cli_flag;

__attribute__((__flatten__, __always_inline__, __unused__, __nonnull__(1), __pure__))
inline static unsigned __int128	pdl__pack_arg(const t_nat_str *__restrict__ const arg, const t_size len)
{
	const t_u8			mid		= (t_u8)xft_tern(7 < len, 4, 0);
	const t_u8			tail	= (t_u8)(len - 4);
	unsigned __int128	x		= 0;

	x |= *(const t_u32 *__restrict__ const)arg;
	x |= (unsigned __int128)(*(const t_u32 *__restrict__ const)(arg + mid)) << (mid << 3);
	x |= (unsigned __int128)(*(const t_u32 *__restrict__ const)(arg + tail)) << (tail << 3);
	return (x);
}

__attribute__((__flatten__, __always_inline__, __unused__, __nonnull__(1, 2)))
inline static void	pdl__report(t_writer *__restrict__ const writer, const char *__restrict__ const msg, const t_size len)
{
	xft_writer_write(writer, xft_fatptr((t_any)msg, len));
	xft_writer_flush(writer);
}

__attribute__((__flatten__, __always_inline__, __unused__, __nonnull__(1)))
inline static void	pdl__compiler_destroy(t_compiler_data *__restrict__ const data)
{
	if (__builtin_expect(data->src_buf.mem != nullptr, 1))		xft_unmap_file(data->src_buf);
	if (__builtin_expect(data->err_state.is_init, 0))			xft_vec_destroy(data->allocator, &data->err_state.line_offsets);
	if (__builtin_expect(data->tokens_soa.ptr != nullptr, 1))	xft_destroy_soa(data->allocator, &data->tokens_soa);

	if (__builtin_expect(data->symbols.scope_map_array.buf.mem != nullptr, 1))
	{
		if (__builtin_expect(data->symbols.scope_depth > 0, 1))	pdl_pop_namespace(&data->symbols);
																pdl_destroy_symbol_table(data->allocator, &data->symbols);
	}

	if (__builtin_expect(data->gpa.slab != nullptr, 1))			xft_gpa_destroy(&data->gpa);
}

__attribute__((__flatten__, __always_inline__, __unused__, __nonnull__(1, 2)))
static inline  t_result	pdl__compiler_init(t_compiler_data *__restrict__ const data, const t_compiler_options *__restrict__ const opts)
{
	data->err_state.is_init = false;
	data->gpa = xft_new_gpa();
	if (__builtin_expect(data->gpa.slab == nullptr, 0))
		return (pdl__report(&data->_stderr, ERR_ALLOC_INIT, sizeof(ERR_ALLOC_INIT) - 1), KO);
	data->allocator = xft_gpa_allocator(&data->gpa);

	data->src_buf = xft_map_file(opts->src_file.mem, O_RDONLY);
	if (__builtin_expect(data->src_buf.mem == nullptr, 0))
		return (pdl__report(&data->_stderr, ERR_CANNOT_MAP_FILE, sizeof(ERR_CANNOT_MAP_FILE) - 1), KO);

	data->tokens_soa = xft_new_soa(data->allocator, 512, &token_soa_ctx);
	if (__builtin_expect(data->tokens_soa.ptr == nullptr, 0))
		return (pdl__report(&data->_stderr, ERR_TOKENS_SOA_OOM, sizeof(ERR_TOKENS_SOA_OOM) - 1), KO);

	data->symbols = pdl_new_symbol_table(data->allocator);
	if (__builtin_expect(data->symbols.scope_map_array.buf.mem == nullptr, 0))
		return (pdl__report(&data->_stderr, ERR_SYMBOLS_OOM, sizeof(ERR_SYMBOLS_OOM) - 1), KO);

	// @Todo: no parser yet, so everything goes into a single namespace opened here.
	if (__builtin_expect(pdl_push_scope(data->allocator, &data->symbols) == KO, 0))
		return (pdl__report(&data->_stderr, ERR_SYMBOLS_OOM, sizeof(ERR_SYMBOLS_OOM) - 1), KO);

	data->err_state.writer = &data->_stderr;
	data->lexer_state = (t_lexer_state){.offset = 0};
	data->filename = opts->src_file;
	return (OK);
}

// lexes the whole file; until the parser exists this also fills the symbol table (the hack)
__attribute__((__nonnull__(1)))
static t_result	pdl__compiler_run(t_compiler_data *__restrict__ const data)
{
	t_result	result;

	while ((result = pdl_lex_next_token(data->allocator, data->src_buf, data->filename, &data->lexer_state,
				&data->tokens_soa, &data->err_state)) == OK)
	{
		const t_size									last = data->tokens_soa.occupied_elements - 1;
		const t_token_type *__restrict__ const			ttype = xft_soa_fields(&data->tokens_soa, &token_soa_ctx, TOKEN_SOA_TYPE);
		const t_token_body_union *__restrict__ const	tvalues = xft_soa_fields(&data->tokens_soa, &token_soa_ctx, TOKEN_SOA_AS);
		t_buffer										name;

		if (__builtin_expect(ttype[last] == TOK_EOF, 0)) break;
		if (ttype[last] != TOK_IDENTIFIER) continue;

		// @HACK: no parser yet, every identifier is interned as a simple int on first sight, we can't know anything else
		// and since we've already done the full table well this is a hack.

		name = xft_fatptr(data->src_buf.mem + tvalues[last].lexeme.start, tvalues[last].lexeme.end - tvalues[last].lexeme.start);
		if (pdl_get_symbol_from_current_namespace_up(&data->symbols, name, false).is_some) continue;
		if (__builtin_expect(pdl_intern_simple_symbol_to_current_namespace(data->allocator, &data->symbols,
				tvalues, data->src_buf, last, SYM_INT) == KO, 0))
		{
			pdl__report(&data->_stderr, ERR_SYMBOLS_OOM, sizeof(ERR_SYMBOLS_OOM) - 1);
			return (KO);
		}
	}
	return (result);
}

__attribute__((__nonnull__(1, 2)))
inline static t_result	pdl__compiler_dump(t_compiler_data *__restrict__ const data, const t_compiler_options *__restrict__ const opts)
{
	const t_token_type *__restrict__ const			ttype = xft_soa_fields(&data->tokens_soa, &token_soa_ctx, TOKEN_SOA_TYPE);
	const t_token_body_union *__restrict__ const	tvalues = xft_soa_fields(&data->tokens_soa, &token_soa_ctx, TOKEN_SOA_AS);
	static t_u8										buffer[1024];
	t_writer										out;
	int												fd;

	if (ttype == nullptr)	__builtin_unreachable();
	if (tvalues == nullptr) __builtin_unreachable();

	if (opts->dump_tokens)
	{
		fd = xft_open((const char *)opts->token_file.mem, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (__builtin_expect(fd == -1, 0))
			return (pdl__report(&data->_stderr, ERR_CANNOT_OPEN_OUTFILE, sizeof(ERR_CANNOT_OPEN_OUTFILE) - 1), KO);
		out = xft_get_fs_writer(xft_fatptr(buffer, sizeof(buffer)), fd);
		for (t_size i = 0; i < data->tokens_soa.occupied_elements; i++)
		{
			t_buffer				str;
			t_u64a					args[4];
			t_symbol_lookup_result	sym;

			str = pdl_token_type_to_string(ttype[i]);
			args[0] = (t_uptr)str.mem;
			args[1] = str.size;
			switch (ttype[i])
			{
				case TOK_IDENTIFIER:
					str = xft_fatptr(data->src_buf.mem + tvalues[i].lexeme.start, tvalues[i].lexeme.end - tvalues[i].lexeme.start);
					sym = pdl_get_symbol_from_current_namespace_up(&data->symbols, str, false);

					if (__builtin_expect(sym.is_some == 0, 0)) __builtin_trap(); // just a hack for now

					args[2] = (t_u_sym_32){.sym = sym.sym}.u32;
					xft_fmt_writer(&out, xft_fatptr((t_any)"<%s, %q>\n", sizeof("<%s, %q>\n") - 1), (t_any)&args);
					break;
				case TOK_STRING_LITERAL:
					args[2] = (t_uptr)data->src_buf.mem + tvalues[i].lexeme.start;
					args[3] = (t_u64a)(tvalues[i].lexeme.end - tvalues[i].lexeme.start);
					xft_fmt_writer(&out, xft_fatptr((t_any)"<%s, \"%s\">\n", sizeof("<%s, \"%s\">\n") - 1), (t_any)&args);
					break;
				case TOK_CONST_INT:
					args[2] = tvalues[i].value.i;
					xft_fmt_writer(&out, xft_fatptr((t_any)"<%s, %q>\n", sizeof("<%s, %q>\n") - 1), (t_any)&args);
					break;
				case TOK_CONST_FLOAT:
					// args is u64s, underneath fmt reinterprets again, safe
					args[2] = tvalues[i].value.i;
					xft_fmt_writer(&out, xft_fatptr((t_any)"<%s, %f>\n", sizeof("<%s, %f>\n") - 1), (t_any)&args);
					break;
				default:
					xft_fmt_writer(&out, xft_fatptr((t_any)"<%s,>\n", sizeof("<%s,>\n") - 1), (t_any)&args);
			}
		}
		xft_writer_flush(&out);
		xft_close(fd);
	}

	if (opts->dump_tables)
	{
		fd = xft_open((const char *)opts->table_file.mem, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (__builtin_expect(fd == -1, 0))
			return (pdl__report(&data->_stderr, ERR_CANNOT_OPEN_OUTFILE, sizeof(ERR_CANNOT_OPEN_OUTFILE) - 1), KO);
		out = xft_get_fs_writer(xft_fatptr(buffer, sizeof(buffer)), fd);
		pdl_dump_current_namespace(&out, &data->symbols, tvalues, data->src_buf);
		xft_writer_flush(&out);
		xft_close(fd);
	}
	return (OK);
}

__attribute__((__nonnull__(1, 2)))
static t_result	pdl__compiler_drive(t_compiler_data *__restrict__ const data, const t_compiler_options *__restrict__ const opts)
{
	t_result	result;

	result = pdl__compiler_init(data, opts);
	if (__builtin_expect(result == OK, 1)) result = pdl__compiler_run(data);
	if (__builtin_expect(result == OK, 1)) result = pdl__compiler_dump(data, opts);
	pdl__compiler_destroy(data);
	return (result);
}

__attribute__((__nonnull__(1), noreturn))
void	xft_main(const t_any *__restrict__ const sp)
{
	static t_kernel_ptrs		ptrs;
	static t_compiler_data		data;
	static t_compiler_options	opts;
	t_size						it;
	t_size						len;
	t_compiler_cli				cli;
	t_cli_flag					flag;
	t_result					result;

	ptrs = xft_get_kernel_ptrs(sp);
	data._stderr = xft_get_fs_writer(xft_fatptr(data._stderr_buffer, sizeof(data._stderr_buffer)), xft_get_stderr());
	cli = NEUTRAL;
	opts.src_file.mem = nullptr;
	it = 1;
	while (it < ptrs.argc)
	{
		len = xft_strlen((t_any)ptrs.argv[it]);
		switch (cli)
		{
			case NEUTRAL: break;
			case EXPECT_TOKEN_FILE:
			{
				opts.token_file = xft_fatptr(ptrs.argv[it++], len);
				cli = NEUTRAL;
				continue;
			}
			case EXPECT_TABLE_FILE:
			{
				opts.table_file = xft_fatptr(ptrs.argv[it++], len);
				cli = NEUTRAL;
				continue;
			}
			case EXPECT_AST_FILE:
			{
				opts.ast_file = xft_fatptr(ptrs.argv[it++], len);
				cli = NEUTRAL;
				continue;
			}
		}
		flag = FLAG_NONE;
		if (len - 5 < 5) // (len >= 5 && len <= 9)
		{
			const unsigned __int128	x = pdl__pack_arg(ptrs.argv[it], len);
			const t_u32a			idx = (t_u32a)representative_bits_hash(x);

			flag = (t_cli_flag)xft_tern(TABLE_REPR[idx] == x, idx, FLAG_NONE);
		}
		switch (flag)
		{
			case FLAG_HELP:
			{
				pdl__report(&data._stderr, ERR_USAGE, sizeof(ERR_USAGE) - 1);
				xft_exit(0);
			}
			case FLAG_TOKENS:
			{
				opts.dump_tokens = true;
				cli = EXPECT_TOKEN_FILE;
				break;
			}
			case FLAG_SYMBOLS:
			{
				opts.dump_tables = true;
				cli = EXPECT_TABLE_FILE;
				break;
			}
			case FLAG_AST:
			{
				opts.dump_ast = true;
				cli = EXPECT_AST_FILE;
				break;
			}
			default:
				if (__builtin_expect(opts.src_file.mem == nullptr, 1)) opts.src_file = xft_fatptr(ptrs.argv[it], len);
				else
				{
					pdl__report(&data._stderr, ERR_MULTIPLE_SOURCES, sizeof(ERR_MULTIPLE_SOURCES) - 1);
					xft_exit(1);
				}
		}
		++it;
	}
	if (__builtin_expect(cli != NEUTRAL, 0))
	{
		pdl__report(&data._stderr, ERR_MISSING_FLAG_VALUE, sizeof(ERR_MISSING_FLAG_VALUE) - 1);
		xft_exit(1);
	}

	if (__builtin_expect(opts.src_file.mem == nullptr, 0))
	{
		pdl__report(&data._stderr, ERR_USAGE, sizeof(ERR_USAGE) - 1);
		xft_exit(1);
	}

	result = pdl__compiler_drive(&data, &opts);
	xft_writer_flush(&data._stderr);
	xft_exit(result == KO);
}
