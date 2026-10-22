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

// REPRESENTACION DE MEMORIA:
//
//	t_symbol
//		|
//		|- type == SIMPLE -> Vec<simple_types>[index_to_dense_array] -> t_symbols_simple
//		|
//		|- type == FUNC -> Vec<functions>[index_to_dense_array] -> t_symbol_func
//
//	Para los nodos de tamaño variable (argumentos de funciones), vamos a tener un
//	array Vec<Tipo_Sym_Simple>. entonces la funcion guarda [comienzo_de_args, fin_de_args),
//	que indexan  al array que acabamos de definir y así el type checking resuelve bien.
//
//  En paralelo tenemos un único mapa para todos los scopes, la semilla del hash es el scope.
//
//	Mapeo:
//
//		mapa
//			|- IndexMap<(scope, fatptr<nombre>), t_symbol>
//
//	El t_symbol.type dice a que array de los dos tenemos que acceder, y los 31 bits restantes
//	dicen el índice a dicho array.
//
//	Aqui no tiene sentido usar Soas por que cuando hagamos type checking vamos a tocar todos
//	los campos.
//
//	@IMPORTANTE: NO PODEMOS USAR PUNTEROS, mientras los arrays crecen las referencias se invalidan
//				 _NECESITAMOS ESTABILIDAD REFERENCIAL_.
//
//	@TODO: quitar el símbolo del token cuando esto esté implementado, es redundante y acopla de más,
//	la jerarquía de conocimiento es Tabla de simbolos -> tokens y no al revés, el lexer produce el símbolo
//	o lo que sea que piden en el diseño de la asignatura, pero el flujo de informacion va de arriba abajo
//	y no al revés. Sólo cambia quien es el owner de la info.
//
//	Ahora bien, hay que ver si usamos un stack de mapas o no, si el lenguaje admite usos antes que
//	declaraciones (de funciones) hay que hacer dos pases:
//
//		-	construccion
//		-	análisis
//
//	si no podemos ir haciendo pop() al vec de los mapas despues de volcar los datos o lo que sea.
//	Las funciones solo se declaran en el namespace 0, que nunca se hace pop(), así que siguen
//	siendo alcanzables por el mapa; el lookup de una función va directamente al namespace 0.
//	simple_types es una pila: cada simple guarda su scope, así que los del scope de arriba son
//	la cola del vec. Al hacer pop() se borran del mapa y se trunca simple_types. function_types y
//	arg_types solo crecen: las funciones y sus runs de argumentos tienen que seguir siendo
//	válidos para el type checking.

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
