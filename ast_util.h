#ifndef AST_UTIL_H
#define AST_UTIL_H

#include <stdio.h>

#ifdef ENABLE_TRACE
#define TRACE(format, ...) fprintf(stderr, "[%s:%d] " format "\n", __FILE__, __LINE__, __VA_ARGS__)
#else
#define TRACE(format, ...) 
#endif

#endif//AST_UTIL_H