#pragma once

#include <xft.h>

typedef enum e_token_type {
	TOK_EOF = '\0',

	TOK_STRING_LITERAL,
	TOK_BOOL,
	TOK_DO,
	TOK_ECHO,
	TOK_FLOAT,
	TOK_FUNC,
	TOK_IF,
	TOK_INT,
	TOK_LET,
	TOK_PROMPT,
	TOK_RETURN,
	TOK_STRING,
	TOK_VOID,
	TOK_WHILE,
	TOK_CONST_FLOAT,
	TOK_CONST_INT,
	TOK_IDENTIFIER,

	TOK_EQ = '=',
	TOK_COMMA = ',',
	TOK_SEMI = ';',
	TOK_LPAREN = '(',
	TOK_RPAREN = ')',
	TOK_LBRACE = '{',
	TOK_RBRACE = '}',

	TOK_ADD = '+',
	TOK_MUL = '*',
	TOK_DIV = '/',
	TOK_SUB = '-',
	TOK_REM = '%',

	TOK_AND,
	TOK_OR,
	TOK_BANG = '!',

	TOK_LT = '<',
	TOK_GT = '>',
	TOK_LE,
	TOK_GE,
	TOK_NE,
	TOK_EQEQ,

	TOK_ADDEQ,
	
	TOK_TRUE,
	TOK_FALSE,
}	t_token_type;

typedef struct s_token
{
	t_token_type	type;
	union {
		struct
		{
			t_u32a	start;
			t_u32a	end;
		}	lexeme;
		t_u64a value;
	} 				as;
}	t_token;


typedef struct s_lexer_state {
	t_u32a offset;

t_result		lex_next_token(t_allocator allocator, t_buffer src, t_vec* __restrict__ const vec);