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

	TOK_LT		= '<',
	TOK_GT		= '>',
	TOK_LE		= '<' | 0x80,
	TOK_GE		= '>' | 0x80,
	TOK_NE		= '!' | 0x80,
	TOK_EQEQ	= '=' | 0x80,

	TOK_ADDEQ	= '+' | 0x80,
	
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
	t_u32a			symbol;
}	t_token;

enum e_token_soa_field {
	TOKEN_SOA_TYPE,
	TOKEN_SOA_AS,
	TOKEN_SOA_SYMBOL,
	TOKEN_SOA_FIELDS_COUNT,
};

#define TOKEN_SOA_TYPE_SIZE		sizeof(((t_token *)0)->type)
#define TOKEN_SOA_AS_SIZE		sizeof(((t_token *)0)->as)
#define TOKEN_SOA_SYMBOL_SIZE	sizeof(((t_token *)0)->symbol)

extern const t_soa_context	token_soa_ctx;
