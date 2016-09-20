#ifndef AST_H
#define AST_H

#include "php.h"
#include "zend_ast.h"
#include "zend_arena.h"

typedef struct _ast_tree {
	zend_ast* root;
	zend_arena* arena;
	int refcount;
}ast_tree;

typedef struct _ast_object {
	zend_ast* node;
	ast_tree* tree;
	zend_object std;
}ast_object;

static inline ast_object* ast_object_from_zend_object(zend_object* zobj_p) {
	return (ast_object*)((char*)zobj_p - XtOffsetOf(ast_object, std));
}

static inline zend_object* zend_object_from_ast_object(ast_object* astobj_p) {
	return &(astobj_p->std);
}

static inline ast_object* AST_OBJ_P(zval* zval_p) {
	return ast_object_from_zend_object(Z_OBJ_P(zval_p));
}

#endif//AST_H
