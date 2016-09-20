#include "php_ast.h"
#include "ast.h"
#include "ast_util.h"

#include "zend_ast.h"
#include "zend_language_scanner.h"
#include "zend_language_parser.h" 

#if PHP_MAJOR_VERSION < 7
# error AST requires PHP version 7 or newer
#endif

#define AST_NO_PROCESS 1

zend_class_entry* ast_ce_p = NULL;
zend_class_entry* ast_node_ce_p = NULL;
zend_class_entry* ast_decl_ce_p = NULL;
zend_class_entry* ast_list_ce_p = NULL;
zend_class_entry* ast_zval_ce_p = NULL;

static zend_object_handlers ast_node_handlers;


void(*prev_ast_process)(zend_ast *ast);
extern ZEND_API zend_ast_process_t zend_ast_process;

////////////////////////////////////////////////////////////
//
void ast_create_object(zval* return_value, zend_ast* ast_p, ast_tree* tree_p, zend_bool zval_as_value_b) {
	zval* cached_obj_p;
	zend_class_entry* ce_p = NULL;

	if (!ast_p) {
		RETURN_NULL();
	}

	//TODO cached objects

	switch (ast_p->kind) {
	case ZEND_AST_ZVAL:
		if (zval_as_value_b) {
			zval* zval_p = zend_ast_get_zval(ast_p);
			RETURN_ZVAL(zval_p, 1, 0);
		}
		ce_p = ast_zval_ce_p;
		break;

#define AST(id)
#define AST_DECL(id) case ZEND_AST_##id: ce_p=ast_decl_ce_p;break;
#define AST_LIST(id) case ZEND_AST_##id: ce_p=ast_list_ce_p;break;
#define AST_CHILD(id, children) case ZEND_AST_##id: ce_p = ast_node_ce_p;break;
#include "ast_kinds.h"
#undef AST_CHILD
#undef AST_LIST
#undef AST_DECL
#undef AST
	default:
		php_error_docref(NULL, E_WARNING, "Unknown AST node kind: %d", (int)ast_p->kind);
		RETURN_NULL();
	}

	ast_object* ast_obj_p;
	object_init_ex(return_value, ce_p);
	ast_obj_p = AST_OBJ_P(return_value);
	ast_obj_p->node = ast_p;
	ast_obj_p->tree = tree_p;
	++tree_p->refcount;

}

void ast_destroy(zend_ast* ast_p) {
	zval* node_p; // the node corresponding to the specified ast
	if (!ast_p) {
		return;
	}

	// TODO deal with cache

	if (ast_p->kind == ZEND_AST_ZVAL) {
		zend_ast_destroy(ast_p);
		return;
	}
	if (ast_p->kind == ZEND_AST_ZNODE) {
		php_error_docref(NULL, E_WARNING, "Encountered unexpected AST_ZNODE");
	}
	if (zend_ast_is_list(ast_p)) {
		zend_ast_list* list_p = zend_ast_get_list(ast_p);
		uint32_t i;
		for (i = 0; i < list_p->children; i++) {
			ast_destroy(list_p->child[i]);
		}
		list_p->children = 0;
		zend_ast_destroy(list_p);
		return;
	}
	if (ast_p->kind < (1 << ZEND_AST_IS_LIST_SHIFT)) {
		uint32_t i;
		zend_ast_decl* decl_p = (zend_ast_decl*)ast_p;
		for (i = 0; i < 4; i++) {
			ast_destroy(decl_p->child[i]);
			decl_p->child[i] = NULL;
		}
		zend_ast_destroy(decl_p);
		return;
	}

	uint32_t i;
	uint32_t children = zend_ast_get_num_children(ast_p);
	for (i = 0; i < children; i++) {
		ast_destroy(ast_p->child[i]);
		ast_p->child[i] = NULL;
	}
	zend_ast_destroy(ast_p);
}

zend_ast* ast_copy(zend_ast* ast_p) {

}


////////////////////////////////////////////////////////////
// Ast class 

/* {{{ proto Ast:__construct() */
static PHP_METHOD(Ast, __construct) {

}/* }}} */

 /* {{{ proto Ast::parseString(string $code, int $options) */
ZEND_BEGIN_ARG_INFO(ast_parsestring_arginfo, 0)
ZEND_ARG_INFO(0, code)
ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

static PHP_METHOD(Ast, parseString) {
	zend_string* source_zstr_p;
	zend_long options_l;
	zval source_zval;
	zend_lex_state original_lexical_state;
	ast_tree* tree_p;
	int parse_result_n;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|l", &source_zstr_p, &options_l) == FAILURE) {
		RETURN_NULL();
	}

	zend_save_lexical_state(&original_lexical_state);
	ZVAL_STR_COPY(&source_zval, source_zstr_p);
	if (zend_prepare_string_for_scanning(&source_zval, "Ast:parseString()") == FAILURE) {
		zend_restore_lexical_state(&original_lexical_state);
		RETURN_NULL();
	}

	CG(ast) = NULL;
	CG(ast_arena) = zend_arena_create(1024 * 32);
	parse_result_n = zendparse();
	zval_dtor(&source_zval);

	if (parse_result_n != 0) {
		zend_ast_destroy(CG(ast));
		zend_arena_destroy(CG(arena));
		CG(ast) = NULL;
		CG(arena) = NULL;
		zend_restore_lexical_state(&original_lexical_state);
		RETURN_NULL();
	}

	if (zend_ast_process && !(options_l&AST_NO_PROCESS)) {
		zend_ast_process(CG(ast));
	}

	tree_p = emalloc(sizeof(ast_tree));
	tree_p->root = CG(ast);
	tree_p->arena = CG(ast_arena);
	tree_p->refcount = 0;

	CG(ast) = NULL;
	CG(ast_arena) = NULL;
	zend_restore_lexical_state(&original_lexical_state);
	ast_create_object(return_value, tree_p->root, tree_p, 0);
}/* }}} */




static zend_function_entry ast_methods[] = {
	PHP_ME(Ast, __construct, NULL, ZEND_ACC_PRIVATE | ZEND_ACC_CTOR)
	PHP_ME(Ast, parseString, ast_parsestring_arginfo, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_FE_END
};

int ast_minit(INIT_FUNC_ARGS) {
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "Ast", ast_methods);
	ast_ce_p = zend_register_internal_class(&ce);
	return SUCCESS;
};



////////////////////////////////////////////////////////////
// AstNode class

static zend_object* ast_node_create(zend_class_entry* ce_p) {
	TRACE("creating ast node...");
	ast_object* astobj_p = ecalloc(1, sizeof(ast_object) + zend_object_properties_size(ce_p));
	zend_object* zobj_p = zend_object_from_ast_object(astobj_p);
	zend_object_std_init(zobj_p, ce_p);
	zobj_p->handlers = &ast_node_handlers;
	object_properties_init(zobj_p, ce_p);
	return zobj_p;
}

static zend_object* ast_node_clone(zval* zval_p) {
	TRACE("cloning ast node : TODO");
	return NULL;
}

static void ast_node_free(zend_object* zobj_p) {
	TRACE("freeing ast node");

	ast_object* astobj_p = ast_object_from_zend_object(zobj_p);
	//TODO remove from cache

	ast_destroy(astobj_p->node);

	if (astobj_p->tree) {
		if (--astobj_p->tree->refcount <= 0) {
			zend_ast_destroy(astobj_p->tree->root);
			zend_arena_destroy(astobj_p->tree->arena);
			efree(astobj_p->tree);
			astobj_p->tree = NULL;
		}
	}
	astobj_p = NULL;
	zend_object_std_dtor(zobj_p);
}


static PHP_METHOD(AstNode, export) {
	ast_object* this_astobj_p;
	this_astobj_p = AST_OBJ_P(getThis());
	zend_string* result_zstr_p = zend_ast_export("", this_astobj_p->node, "");
	if (result_zstr_p) {
		RETURN_STR(result_zstr_p);
	}
	RETURN_EMPTY_STRING();
}


static zend_function_entry ast_node_methods[] = {
	PHP_ME(AstNode, export, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

int ast_node_minit(INIT_FUNC_ARGS) {
	zend_class_entry ce;

	// initialize the handlers for the ast_node instances
	memcpy(&ast_node_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	ast_node_handlers.offset = XtOffsetOf(ast_object, std);
	ast_node_handlers.free_obj = ast_node_free;
	ast_node_handlers.clone_obj = ast_node_clone;

	INIT_CLASS_ENTRY(ce, "AstNode", ast_node_methods);
	ast_node_ce_p = zend_register_internal_class(&ce);
	ast_node_ce_p->create_object = ast_node_create;



	return SUCCESS;
}

////////////////////////////////////////////////////////////
// AstDecl class 

static zend_function_entry ast_decl_methods[] = {
	PHP_FE_END
};

int ast_decl_minit(INIT_FUNC_ARGS) {
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "AstDecl", ast_decl_methods);
	ast_decl_ce_p = zend_register_internal_class_ex(&ce, ast_node_ce_p);
	return SUCCESS;
}

////////////////////////////////////////////////////////////
// AstList class

static zend_function_entry ast_list_methods[] = {
	PHP_FE_END
};

int ast_list_minit(INIT_FUNC_ARGS) {
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "AstList", ast_list_methods);
	ast_list_ce_p = zend_register_internal_class_ex(&ce, ast_node_ce_p);
	return SUCCESS;
}

////////////////////////////////////////////////////////////
// AstZval class

static zend_function_entry ast_zval_methods[] = {
	PHP_FE_END
};

int ast_zval_minit(INIT_FUNC_ARGS) {
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "AstZval", ast_zval_methods);
	ast_zval_ce_p = zend_register_internal_class_ex(&ce, ast_node_ce_p);
	return SUCCESS;
}

////////////////////////////////////////////////////////////
//

static void ast_process(zend_ast* ast_p) {
	ZEND_ASSERT(ast_p == CG(ast));
	TRACE("processing ast:");
	if (prev_ast_process) {
		prev_ast_process(ast_p);
	}

}

////////////////////////////////////////////////////////////
// AST MODULE DEFINITION

ZEND_DECLARE_MODULE_GLOBALS(ast);

zend_module_entry ast_module_entry = {
	STANDARD_MODULE_HEADER,
	"ast",
	NULL, /* Functions */
	PHP_MINIT(ast),
	PHP_MSHUTDOWN(ast), /* MSHUTDOWN */
	NULL, /* RINIT */
	NULL, /* RSHUTDOWN */
	NULL, /* MINFO */
	"1.0.0-dev",
	PHP_MODULE_GLOBALS(ast),
	PHP_GINIT(ast), //GINIT
	NULL, /*GSHUTDOWN*/
	NULL, /*RPOSTSHUTDOWN*/
	STANDARD_MODULE_PROPERTIES_EX
};

PHP_MINIT_FUNCTION(ast) {
	prev_ast_process = zend_ast_process;
	zend_ast_process = ast_process;

	return ((ast_minit(INIT_FUNC_ARGS_PASSTHRU) == SUCCESS) &&
		(ast_node_minit(INIT_FUNC_ARGS_PASSTHRU) == SUCCESS) &&
		(ast_decl_minit(INIT_FUNC_ARGS_PASSTHRU) == SUCCESS) &&
		(ast_list_minit(INIT_FUNC_ARGS_PASSTHRU) == SUCCESS) &&
		(ast_zval_minit(INIT_FUNC_ARGS_PASSTHRU) == SUCCESS)) ? SUCCESS : FAILURE;
};

PHP_MSHUTDOWN_FUNCTION(ast) {
	zend_ast_process = prev_ast_process;
	return SUCCESS;
};


PHP_GINIT_FUNCTION(ast) {
#if defined(COMPILE_DL_AST) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	zend_hash_init(&ast_globals->nodes, 32, NULL, NULL, 1);
}

#ifdef COMPILE_DL_AST
ZEND_GET_MODULE(ast)
#endif