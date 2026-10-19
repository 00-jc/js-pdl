#pragma once

#include "tokens.h"
#include "errors.h"
#include "lexer.h"
#include "symbols.h"

typedef struct s_compiler_options
{
	t_u8		dump_tokens : 1;
	t_u8		dump_tables : 1;
	t_u8		dump_ast	: 1;

	t_buffer	token_file;
	t_buffer	table_file;
	t_buffer	ast_file;
	t_buffer	src_file;
}	t_compiler_options;

typedef enum e_compiler_cli
{
	EXPECT_TOKEN_FILE,
	EXPECT_TABLE_FILE,
	EXPECT_AST_FILE,
	NEUTRAL,
}	t_compiler_cli;

// everything the analysis pipeline owns; zero-initialised, so teardown can run on a partially built one
typedef struct s_compiler_data
{
	t_gpa			gpa;
	t_allocator		allocator;
	t_writer		_stderr;
	t_u8			_stderr_buffer[1024];
	t_error_state	err_state;
	t_buffer		filename;
	t_buffer		src_buf;
	t_lexer_state	lexer_state;
	t_soa			tokens_soa;
	t_symbol_table	symbols;
}	t_compiler_data;
