#pragma once

/*
 * Silence warnings for GNU computed-goto dispatch tables, i.e.
 *   static const void *t[] = { [0 ... N] = &&a, ['x'] = &&b };
 * (range designators, labels-as-values, overriding initializers).
 */
#if defined(__clang__)
# define DIAG_JUMP_TABLE_BEGIN \
	_Pragma("clang diagnostic push") \
	_Pragma("clang diagnostic ignored \"-Wgnu-designator\"") \
	_Pragma("clang diagnostic ignored \"-Wgnu-label-as-value\"") \
	_Pragma("clang diagnostic ignored \"-Winitializer-overrides\"")
# define DIAG_JUMP_TABLE_END \
	_Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
# define DIAG_JUMP_TABLE_BEGIN \
	_Pragma("GCC diagnostic push") \
	_Pragma("GCC diagnostic ignored \"-Wpedantic\"") \
	_Pragma("GCC diagnostic ignored \"-Woverride-init\"")
# define DIAG_JUMP_TABLE_END \
	_Pragma("GCC diagnostic pop")
#else
# define DIAG_JUMP_TABLE_BEGIN
# define DIAG_JUMP_TABLE_END
#endif
