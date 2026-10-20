#pragma once

#include <xft.h>
#include "symbols.h"
#include "tokens.h"
#include "lexer.h"

// todo: expand this once we grow
// the possible states, we are using
// a packed heap stack not the system's one.
typedef enum e_parse_state : t_u8
{
	EXPR_START,
	EXPR_END,
	STMT_START,
	STMT_END,
}	t_parse_state;

typedef enum e_node_type : t_u8
{
	NODE_STMT_FUNCDECL,
	NODE_STMT_LETASSIGN,

	NODE_STMT_BLOCK,
	NODE_STMT_IF,
	NODE_STMT_RETURN,
	NODE_STMT_DOWHILE,

	NODE_EXPR_CALL,
	NODE_EXPR_BINOP,
	NODE_EXPR_UNOP,
	NODE_EXPR_LEAF,
}	t_node_type;

// todo: soa context for this.

typedef union u_node_body
{
	struct
	{
		// we don't need to store the ast node in the symbol
		//
		// (we might not need this node at all actually)
		//
		// since the symbol already has all arguments, name and
		// whatnot, this node really is just so we can tie a body
		// to a symbol pretty much.
		//
		// Another approach we could explore it to put a ast_node_body
		// in the symbol and remove the body from here so lookups
		// from the symbol table already return it and stores it as
		// the src of truth (more queryable?).
		t_symbol	sym;
		t_u32a		body;
	}	funcdecl;

	struct
	{
		t_u32a	inner;
	}	unop;

	struct
	{
		t_u32a	idx_to_start_stm;
		t_u32a	idx_to_end_stm;
	}	block;

	struct
	{
		t_u32a	inner;
	}	ret;

	struct
	{
		t_u32a	lhs;
		t_u32a	rhs;
	}	binop;

	struct
	{
		t_u32a	cond;
		t_u32a	true_branch;
	}	_if;

	struct
	{
		t_u32a	cond;
		t_u32a	body;
	}	do_while;

	struct
	{
		t_u32a	name;
		t_u32a	idx_to_args;
	}	call;

	struct
	{
		t_u32a	idx_to_token;
	}	leaf;

}	t_node_body;

typedef struct s_node
{
	t_node_type		type;
	t_node_body		as;
}	t_node;

typedef struct s_parser_data
{
	t_soa		nodes;
	t_vec		args_aux;
}	t_parser_data;

typedef struct s_parser
{
	t_lexer_state	lexer;
	t_vec			state_stack;
}	t_parser;
