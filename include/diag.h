#pragma once

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

#define DIAG_GOTO(target) \
	DIAG_JUMP_TABLE_BEGIN \
	goto *(target); \
	DIAG_JUMP_TABLE_END

#if defined(__clang__)
# define DIAG_FAM_INIT_BEGIN \
	_Pragma("clang diagnostic push") \
	_Pragma("clang diagnostic ignored \"-Wgnu-flexible-array-initializer\"")
# define DIAG_FAM_INIT_END \
	_Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
# define DIAG_FAM_INIT_BEGIN \
	_Pragma("GCC diagnostic push") \
	_Pragma("GCC diagnostic ignored \"-Wpedantic\"")
# define DIAG_FAM_INIT_END \
	_Pragma("GCC diagnostic pop")
#else
# define DIAG_FAM_INIT_BEGIN
# define DIAG_FAM_INIT_END
#endif
