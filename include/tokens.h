#pragma once

#include <xft.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201710L
# define BACKING_STORAGE_ENUM(t) : t
#elif
# define BACKING_STORAGE_ENUM(t)
#endif

typedef enum e_token_type BACKING_STORAGE_ENUM(t_u8) {
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

	TOK_AND	= '&',
	TOK_OR	= '|',
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

typedef union s_token_body_union
{
	struct
	{
		t_u32a	start;
		t_u32a	end;
	}	lexeme;
	t_dp value;
}	t_token_body_union;

typedef struct s_token
{
	t_token_type		type;
	t_token_body_union 	as;
}	t_token;

enum e_token_soa_field {
	TOKEN_SOA_TYPE,
	TOKEN_SOA_AS,
	TOKEN_SOA_FIELDS_COUNT,
};

#define TOKEN_SOA_TYPE_SIZE		sizeof(((t_token *)0)->type)
#define TOKEN_SOA_AS_SIZE		sizeof(((t_token *)0)->as)

extern const t_soa_context	token_soa_ctx;

__attribute__((__always_inline__, const))
static inline t_buffer	pdl_token_type_to_string(t_token_type tokt)
{
#define TOKT_STR(s) xft_fatptr((t_any)s, sizeof(s) - 1)

	switch (tokt)
	{
		case TOK_EOF:             return (TOKT_STR("EOF"));
		case TOK_STRING_LITERAL:  return (TOKT_STR("STRINGLIT"));
		case TOK_BOOL:            return (TOKT_STR("BOOL"));
		case TOK_DO:              return (TOKT_STR("DO"));
		case TOK_ECHO:            return (TOKT_STR("ECHO"));
		case TOK_FLOAT:           return (TOKT_STR("FLOAT"));
		case TOK_FUNC:            return (TOKT_STR("FUNC"));
		case TOK_IF:              return (TOKT_STR("IF"));
		case TOK_INT:             return (TOKT_STR("INT"));
		case TOK_LET:             return (TOKT_STR("LET"));
		case TOK_PROMPT:          return (TOKT_STR("PROMPT"));
		case TOK_RETURN:          return (TOKT_STR("RETURN"));
		case TOK_STRING:          return (TOKT_STR("STRING"));
		case TOK_VOID:            return (TOKT_STR("VOID"));
		case TOK_WHILE:           return (TOKT_STR("WHILE"));
		case TOK_CONST_FLOAT:     return (TOKT_STR("CONSTFLOAT"));
		case TOK_CONST_INT:       return (TOKT_STR("CONSTINT"));
		case TOK_IDENTIFIER:      return (TOKT_STR("IDENTIFIER"));
		case TOK_EQ:              return (TOKT_STR("EQ"));
		case TOK_COMMA:           return (TOKT_STR("COMMA"));
		case TOK_SEMI:            return (TOKT_STR("SEMI"));
		case TOK_LPAREN:          return (TOKT_STR("LPAREN"));
		case TOK_RPAREN:          return (TOKT_STR("RPAREN"));
		case TOK_LBRACE:          return (TOKT_STR("LBRACE"));
		case TOK_RBRACE:          return (TOKT_STR("RBRACE"));
		case TOK_ADD:             return (TOKT_STR("ADD"));
		case TOK_MUL:             return (TOKT_STR("MUL"));
		case TOK_DIV:             return (TOKT_STR("DIV"));
		case TOK_SUB:             return (TOKT_STR("SUB"));
		case TOK_REM:             return (TOKT_STR("REM"));
		case TOK_AND:             return (TOKT_STR("AND"));
		case TOK_OR:              return (TOKT_STR("OR"));
		case TOK_BANG:            return (TOKT_STR("BANG"));
		case TOK_LT:              return (TOKT_STR("LT"));
		case TOK_GT:              return (TOKT_STR("GT"));
		case TOK_LE:              return (TOKT_STR("LE"));
		case TOK_GE:              return (TOKT_STR("GE"));
		case TOK_NE:              return (TOKT_STR("NE"));
		case TOK_EQEQ:            return (TOKT_STR("EQEQ"));
		case TOK_ADDEQ:           return (TOKT_STR("ADDEQ"));
		case TOK_TRUE:            return (TOKT_STR("TRUE"));
		case TOK_FALSE:           return (TOKT_STR("FALSE"));
	}
	return (TOKT_STR("UNKNOWN"));

#undef TOKT_STR
}
