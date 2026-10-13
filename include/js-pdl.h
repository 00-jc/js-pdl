#pragma once

#include "tokens.h"
#include "errors.h"
#include "lexer.h"
#include "symbols.h"

#define USAGE "usage: <exe> <file.js>\n"
#define ALLOC_INIT_OOM "Cannot initialize allocator, most likely OOM\n"
#define CANNOT_MAP_FILE "Cannot map source file, check that it exists and is readable\n"
#define TOKENS_SOA_OOM "Cannot allocate token storage, most likely OOM\n"
#define LINES_OOM "Cannot allocate line table, most likely OOM\n"
