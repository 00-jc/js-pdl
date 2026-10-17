#pragma once

#include <xft.h>

typedef enum e_symbol_simple_type {
	INT,
	FLOAT,
	STR,
	BOOL
} e_symbol_simple_type;

typedef struct s_symbol_simple {
	e_symbol_simple_type 	type;
	t_u32					despl;
} t_symbol_simple;

typedef struct s_symbol_func {
	t_u32 				nParam;
	t_symbol_simple		*args;
	t_symbol_simple		retType;
	char				*etiq;
	t_u32				despl;
} t_symbol_func;

typedef union u_symbol_attributes {
	t_symbol_simple	simple;
	t_symbol_func	func;
} t_symbol_attributes;

typedef enum e_symbol_type {
	SIMPLE,
	FUNC
} e_symbol_type;

typedef struct s_symbol
{
	t_u32a			scope;
	e_symbol_type 	type;
}	t_symbol;

enum e_symbol_soa_field {
	SYMBOL_SOA_SCOPE,
};

#define SYMBOL_SOA_SCOPE_SIZE		sizeof(((t_symbol *)0)->scope)

extern const t_soa_context	symbol_soa_ctx;
