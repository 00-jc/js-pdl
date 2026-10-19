#include "symbols.h"

// identifier name of a token, used as the key into the scope maps
__attribute__((__always_inline__, pure))
static inline t_buffer	pdl__token_name(const t_buffer src,
	const t_token_body_union *__restrict__ const tokens, t_size idx)
{
	return (xft_fatptr(src.mem + tokens[idx].lexeme.start, tokens[idx].lexeme.end - tokens[idx].lexeme.start));
}

__attribute__((__always_inline__, pure, __returns_nonnull__))
static inline t_indexmap	*pdl__scope_map(const t_symbol_table *__restrict__ const table, t_size scope)
{
	t_scope	*ptr;

	ptr = xft_vec_get_mut(&table->scope_map_array, scope, sizeof(t_scope));
	if (ptr == nullptr) __builtin_unreachable();
	return (&ptr->map);
}

__attribute__((__always_inline__, const))
static inline t_size	pdl__symbol_to_value(t_symbol sym)
{
	return ((t_u_sym_32){.sym = sym}.u32);
}

__attribute__((__always_inline__, const))
static inline t_symbol	pdl__value_to_symbol(t_size value)
{
	return ((t_u_sym_32){.u32 = (t_u32a)value}.sym);
}

#define ERR_NO_NAMESPACE "error: symbol table used with no namespace open\n"

// @Todo: the parser should never touch the table without a namespace open,
// once it's tested only the NDEBUG part remains and we can remove this.
__attribute__((__always_inline__, __flatten__, __unused__))
static inline void	pdl__pin_namespace_open(const t_symbol_table *__restrict__ const table)
{
#ifndef NDEBUG
	xft_pin_invariant_msg(table->scope_depth != 0,
		xft_fatptr((t_any)ERR_NO_NAMESPACE, sizeof(ERR_NO_NAMESPACE) - 1));
#else
	if (table->scope_depth == 0) __builtin_unreachable();
#endif
}

#undef ERR_NO_NAMESPACE

t_symbol_table	pdl_new_symbol_table(t_allocator allocator)
{
	t_symbol_table	table;
	t_scope			scope;

	table.scope_depth = 0;
	table.scope_map_array = xft_new_vec(allocator, 8, sizeof(t_scope));
	table.simple_types = xft_new_vec(allocator, 32, sizeof(t_simple_attributes));
	table.function_types = xft_new_vec(allocator, 16, sizeof(t_function_attributes));
	table.arg_types = xft_new_vec(allocator, 32, sizeof(t_symbol_simple_type));
	if (__builtin_expect(table.scope_map_array.buf.mem == nullptr
		|| table.simple_types.buf.mem == nullptr
		|| table.function_types.buf.mem == nullptr
		|| table.arg_types.buf.mem == nullptr, 0))
	{
		// xft_vec_destroy ignores vecs with no memory
		xft_vec_destroy(allocator, &table.scope_map_array);
		xft_vec_destroy(allocator, &table.simple_types);
		xft_vec_destroy(allocator, &table.function_types);
		xft_vec_destroy(allocator, &table.arg_types);
		return ((t_symbol_table){0});
	}
	for (t_size i = 0; i < SYMBOLS_INITIAL_SCOPES; i++)
	{
		scope = (t_scope){.map = xft_new_indexmap(allocator), .simple_mark = 0};
		if (__builtin_expect(scope.map.soa.ptr == nullptr, 0)
			|| __builtin_expect(xft_vec_push_back(allocator, &table.scope_map_array,
				(const t_u8 *)&scope, sizeof(t_scope)) == KO, 0))
		{
			if (scope.map.soa.ptr != nullptr)
				xft_indexmap_destroy(allocator, &scope.map);
			pdl_destroy_symbol_table(allocator, &table);
			return ((t_symbol_table){0});
		}
	}
	return (table);
}

__attribute__((__nonnull__(2)))
void	pdl_destroy_symbol_table(t_allocator allocator, t_symbol_table *table)
{
	t_scope	*scopes;

	scopes = (t_scope *)table->scope_map_array.buf.mem;
	if (scopes == nullptr) __builtin_unreachable();
	for (t_size i = 0; i < xft_vec_len(&table->scope_map_array, sizeof(t_scope)); i++)
		xft_indexmap_destroy(allocator, &scopes[i].map);
	xft_vec_destroy(allocator, &table->scope_map_array);
	xft_vec_destroy(allocator, &table->simple_types);
	xft_vec_destroy(allocator, &table->function_types);
	xft_vec_destroy(allocator, &table->arg_types);
}

__attribute__((__nonnull__(2)))
t_result	pdl_push_scope(t_allocator allocator, t_symbol_table *table)
{
	t_scope	scope;
	t_scope	*top;

	if (table->scope_depth == xft_vec_len(&table->scope_map_array, sizeof(t_scope)))
	{
		scope = (t_scope){.map = xft_new_indexmap(allocator)};
		if (__builtin_expect(scope.map.soa.ptr == nullptr, 0))
			return (KO);
		if (__builtin_expect(xft_vec_push_back(allocator, &table->scope_map_array,
				(const t_u8 *)&scope, sizeof(t_scope)) == KO, 0))
		{
			xft_indexmap_destroy(allocator, &scope.map);
			return (KO);
		}
	}
	top = xft_vec_get_mut(&table->scope_map_array, table->scope_depth, sizeof(t_scope));
	if (top == nullptr) __builtin_unreachable();
	top->simple_mark = (t_u32a)table->simple_types.size;
	table->scope_depth++;
	return (OK);
}

__attribute__((__nonnull__(1)))
void	pdl_pop_namespace(t_symbol_table *table)
{
	t_scope	*top;

	pdl__pin_namespace_open(table);
	table->scope_depth--;
	top = xft_vec_get_mut(&table->scope_map_array, table->scope_depth, sizeof(t_scope));
	xft_indexmap_clear(&top->map);
	table->simple_types.size = top->simple_mark;
}

__attribute__((__nonnull__(2, 3)))
t_result	pdl_intern_simple_symbol_to_current_namespace(
	t_allocator										allocator,
	t_symbol_table *__restrict__ const				table,
	const t_token_body_union *__restrict__ const	tokens,
	const t_buffer									src,
	t_size											idx_of_token,
	t_symbol_simple_type							typeof_sym
)
{
	const t_buffer		key = pdl__token_name(src, tokens, idx_of_token);
	t_indexmap			*map;
	t_simple_attributes	attr;
	t_size				index;

	pdl__pin_namespace_open(table);

	map = pdl__scope_map(table, table->scope_depth - 1);
	index = xft_vec_len(&table->simple_types, sizeof(t_simple_attributes));
	attr = (t_simple_attributes){
		.relative_offset = (t_u32a)map->count,
		.tag_idx_token = (t_u32a)idx_of_token,
		.type = typeof_sym,
	};

	if (__builtin_expect(xft_vec_push_back(allocator, &table->simple_types,
			(t_any)&attr, sizeof(attr)) == KO, 0))
		return (KO);

	if (__builtin_expect(xft_indexmap_insert(allocator, map, key,
			pdl__symbol_to_value((t_symbol){.type = SYMT_SIMPLE, .index_to_dense_array = (t_u32a)index})) == KO, 0))
	{
		xft_vec_pop(&table->simple_types, sizeof(t_simple_attributes));
		return (KO);
	}

	return (OK);
}

#define ERR_FUNC_NOT_GLOBAL "error: functions can only be declared in the global namespace\n"

__attribute__((__nonnull__(2, 3, 4)))
t_result	pdl_intern_function_symbol_to_global_namespace(
	t_allocator										allocator,
	t_symbol_table *__restrict__ const				table,
	const t_symbol_simple_type *__restrict__ const	args,
	const t_token_body_union *__restrict__ const	tokens,
	const t_buffer									src,
	t_size											idx_of_token,
	t_symbol_simple_type							return_type,
	t_size											nargs
)
{
	const t_buffer			key = pdl__token_name(src, tokens, idx_of_token);
	const t_size			arg_start = xft_vec_len(&table->arg_types, sizeof(t_symbol_simple_type));
	t_indexmap				*map;
	t_function_attributes	attr;
	t_size					index;

	if (key.mem == nullptr) __builtin_unreachable();

	// @Todo: proper diagnostic once the parser reports errors.
	xft_pin_invariant_msg(table->scope_depth == 1,
		xft_fatptr((t_any)ERR_FUNC_NOT_GLOBAL, sizeof(ERR_FUNC_NOT_GLOBAL) - 1));

	map = pdl__scope_map(table, 0);
	index = xft_vec_len(&table->function_types, sizeof(t_function_attributes));
	attr = (t_function_attributes){
		.args = {.start = (t_u32a)arg_start, .end = (t_u32a)(arg_start + nargs)},
		.tag_idx_token = (t_u32a)idx_of_token,
		.return_type = return_type,
	};

	if (__builtin_expect(xft_vec_extend(allocator, &table->arg_types,
			xft_fatptr((t_any)args, nargs * sizeof(t_symbol_simple_type))) == KO, 0))
		return (KO);

	if (__builtin_expect(xft_vec_push_back(allocator, &table->function_types,
			(t_any)&attr, sizeof(attr)) == KO, 0))
		goto rewind_args;

	if (__builtin_expect(xft_indexmap_insert(allocator, map, key,
			pdl__symbol_to_value((t_symbol){.type = SYMT_FUNC, .index_to_dense_array = (t_u32a)index})) == KO, 0))
	{
		xft_vec_pop(&table->function_types, sizeof(t_function_attributes));
		goto rewind_args;
	}

	return (OK);

rewind_args:
	table->arg_types.size = arg_start * sizeof(t_symbol_simple_type);
	return (KO);
}

#undef ERR_FUNC_NOT_GLOBAL

__attribute__((__nonnull__(1)))
t_symbol_lookup_result	pdl_get_symbol_from_current_namespace_up(
	const t_symbol_table *__restrict__ const	table,
	t_buffer									name,
	bool										is_function)
{
	const t_size		depth = table->scope_depth;
	t_size				it;
	t_size				result;

	pdl__pin_namespace_open(table);
	// functions only live in namespace 0
	it = xft_tern(is_function, 1, depth);
	do
	{
		--it;
		result = xft_indexmap_lookup(pdl__scope_map(table, it), name);
		if (__builtin_expect(result != INDEXMAP_NPOS, 1))
			return ((t_symbol_lookup_result){.is_some = 1, .sym = pdl__value_to_symbol(result)});
	}
	while (it);
	return ((t_symbol_lookup_result){.is_some = 0});
}


// @TODO: this is only for the first assignment hack, once we have a proper parser
// we then can do a proper dumper.

#define DUMP_HEADER "#%q\n"
#define DUMP_LEXEMA "+LEXEMA:'%s'\n"

// dumps the innermost namespace: its symbols are [simple_mark, simple_types.size)
__attribute__((__nonnull__(1, 2, 3)))
t_result	pdl_dump_current_namespace(
	t_writer *__restrict__ const					writer,
	const t_symbol_table *__restrict__ const		table,
	const t_token_body_union *__restrict__ const	tokens,
	const t_buffer									src
)
{
	const t_scope				*scope;
	const t_simple_attributes	*attrs;
	t_buffer					name;
	t_u64						args[2];

	pdl__pin_namespace_open(table);
	scope = xft_vec_get_mut(&table->scope_map_array, table->scope_depth - 1, sizeof(t_scope));
	if (scope == nullptr) __builtin_unreachable();
	attrs = (const t_simple_attributes *)table->simple_types.buf.mem;

	args[0] = table->scope_depth - 1;
	if (xft_fmt_writer(writer, xft_fatptr((t_any)DUMP_HEADER, sizeof(DUMP_HEADER) - 1), args) == KO)
		return (KO);
	for (t_size i = scope->simple_mark / sizeof(t_simple_attributes);
		i < xft_vec_len(&table->simple_types, sizeof(t_simple_attributes)); i++)
	{
		name = pdl__token_name(src, tokens, attrs[i].tag_idx_token);
		args[0] = (t_uptr)name.mem;
		args[1] = name.size;
		if (xft_fmt_writer(writer, xft_fatptr((t_any)DUMP_LEXEMA, sizeof(DUMP_LEXEMA) - 1), args) == KO)
			return (KO);
	}
	return (OK);
}

#undef DUMP_HEADER
#undef DUMP_LEXEMA
