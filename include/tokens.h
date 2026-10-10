#pragma once

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
		struct lexeme
		{
			t_u32a	start;
			t_u32a	end;
		};
		t_u64a value;
	} 				as;
}	t_token;
