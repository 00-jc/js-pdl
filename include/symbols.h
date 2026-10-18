#pragma once

#include <xft.h>

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

#ifndef MADE_TEST_SYM
# define MADE_TEST_SYM
static constexpr const t_symbol test______ = {.type = SYMT_FUNC, .index_to_dense_array = 0};
_Static_assert(sizeof(test______) == sizeof(t_u32a), "SYMBOL != U32");
_Static_assert(test______.type == 1, "SYMBOL != U32");
_Static_assert(test______.index_to_dense_array == 0, "SYMBOL != U32");
#endif

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
}	t_simple_attributes;

typedef struct s_scope
{
	t_indexmap	map;
	t_u32a		simple_mark;
	t_u32a		function_mark;
}	t_scope;

typedef struct s_symbol_table
{
	t_size		scope_depth;
	t_vec		scope_map_array;	// vec<t_scope>, pool of scopes; [0, scope_depth) are live
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
//  En paralelo tenemos mapas[], uno por scope.
//
//	Mapeo:
//
//		mapas[scope] (mapa global)
//			|- IndexMap<fatptr<nombre>, t_symbol>
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
//	si quisieramos ademas ahorrar memoria podemos reusar la memoria del resto de vectores al
//	hacer pop(), o si no seguir creciendo los arrays, yo apostaría por la primera (arg_types no se toca).

#define SYMBOLS_INITIAL_SCOPES 25

t_symbol_table	pdl_new_symbol_table(t_allocator allocator);
void			pdl_destroy_symbol_table(t_allocator allocator, t_symbol_table *table)
					__attribute__((__nonnull__(2)));
t_result		pdl_push_scope(t_allocator allocator, t_symbol_table *table)
					__attribute__((__nonnull__(2)));
void			pdl_pop_namespace(t_symbol_table *table)
					__attribute__((__nonnull__(1)));
