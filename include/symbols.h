#pragma once

#include <xft.h>
#include "tokens.h"

typedef enum e_symbol_simple_type : t_u8
{
	SYM_INT,
	SYM_FLOAT,
	SYM_STR,
	SYM_BOOL
}	t_symbol_simple_type;

typedef struct s_symbol
{
	enum
	{
		SYMT_SIMPLE	= 0,
		SYMT_FUNC	= 1,
	} 				type					: 1;
	t_u32a			index_to_dense_array	: 31;
}	t_symbol;

// to make reinterpreting casts stop whining
typedef union u_sym_32
{
	t_symbol	sym;
	t_u32a		u32;
}	t_u_sym_32;

typedef struct s_symbol_lookup_result
{
	bool			is_some;
	t_symbol		sym;
}	t_symbol_lookup_result;

// #ifndef MADE_TEST_SYM
// # define MADE_TEST_SYM
// static constexpr const t_symbol test______ = {.type = SYMT_FUNC, .index_to_dense_array = 0};
// _Static_assert(sizeof(test______) == sizeof(t_u32a), "SYMBOL != U32");
// _Static_assert(test______.type == 1, "SYMBOL != U32");
// _Static_assert(test______.index_to_dense_array == 0, "SYMBOL != U32");
// #endif

typedef struct s_run
{
	t_u32a		start;
	t_u32a		end;
}	t_arg_run;

typedef struct s_function_attributes
{
	t_arg_run				args;
	t_u32a					tag_idx_token;
	t_symbol_simple_type	return_type;
}	t_function_attributes;

typedef struct s_simple_attributes
{
	t_u32a					relative_offset;
	t_u32a					tag_idx_token;
	t_symbol_simple_type	type;
	t_u16a					scope;
}	t_simple_attributes;

typedef struct s_symbol_table
{
	t_size		scope_depth;
	t_indexmap	symbol_map;			// map<(namespace, name), symbol>
	t_vec		simple_types;		// vec<t_simple_attributes>
	t_vec		function_types;		// vec<t_function_attributes>
	t_vec		arg_types;			// vec<t_symbol_simple_type>
}	t_symbol_table;

t_symbol_table				pdl_new_symbol_table(t_allocator allocator);
void						pdl_destroy_symbol_table(t_allocator allocator, t_symbol_table *table)
								__attribute__((__nonnull__(2)));
void						pdl_push_scope(t_symbol_table *table)
								__attribute__((__nonnull__(1)));
void						pdl_pop_namespace(t_symbol_table *table)
								__attribute__((__nonnull__(1)));
t_result					pdl_intern_simple_symbol_to_current_namespace(t_allocator allocator,
								t_symbol_table *table, const t_token_body_union *tokens, const t_buffer src,
								t_size idx_of_token, t_symbol_simple_type typeof_sym)
								__attribute__((__nonnull__(2, 3)));
t_result					pdl_intern_function_symbol_to_global_namespace(t_allocator allocator,
								t_symbol_table *table, const t_symbol_simple_type *args,
								const t_token_body_union *tokens, const t_buffer src,
								t_size idx_of_token, t_symbol_simple_type return_type, t_size nargs)
								__attribute__((__nonnull__(2, 3, 4)));
t_symbol_lookup_result		pdl_get_symbol_from_current_namespace_up(const t_symbol_table *table, t_buffer name,
								bool is_function)
								__attribute__((__nonnull__(1)));
t_result					pdl_dump_current_namespace(t_writer *writer, const t_symbol_table *table,
								const t_token_body_union *tokens, const t_buffer src)
								__attribute__((__nonnull__(1, 2, 3)));
