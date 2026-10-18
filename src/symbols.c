#include "symbols.h"

t_symbol_table	pdl_new_symbol_table(t_allocator allocator)
{
	t_symbol_table	table;
	t_scope			scope;

	table.scope_depth = 0;
	table.scope_map_array = xft_new_vec(allocator, 512, sizeof(t_scope));
	table.simple_types = xft_new_vec(allocator, 512, sizeof(t_simple_attributes));
	table.function_types = xft_new_vec(allocator, 512, sizeof(t_function_attributes));
	table.arg_types = xft_new_vec(allocator, 512, sizeof(t_symbol_simple_type));
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
		scope = (t_scope){.map = xft_new_indexmap(allocator), .function_mark = 0, .simple_mark = 0};
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
	top->simple_mark = table->simple_types.size;
	top->function_mark = table->function_types.size;
	table->scope_depth++;
	return (OK);
}

__attribute__((__nonnull__(1)))
void	pdl_pop_namespace(t_symbol_table *table)
{
	t_scope	*top;

	if (table->scope_depth == 0)
		return ;
	table->scope_depth--;
	top = xft_vec_get_mut(&table->scope_map_array, table->scope_depth, sizeof(t_scope));
	xft_indexmap_clear(&top->map);
	table->simple_types.size = top->simple_mark;
	table->function_types.size = top->function_mark;
}
