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

zend_class_entry* ast_ce= NULL;
zend_class_entry* ast_node_ce= NULL;
zend_class_entry* ast_decl_ce = NULL;
zend_class_entry* ast_list_ce = NULL;
zend_class_entry* ast_zval_ce = NULL;

static zend_object_handlers ast_node_handlers;


void(*prev_ast_process)(zend_ast *ast);
extern ZEND_API zend_ast_process_t zend_ast_process;

////////////////////////////////////////////////////////////
//

static inline size_t zend_ast_size(uint32_t children) {
	return sizeof(zend_ast) - sizeof(zend_ast *) + sizeof(zend_ast *) * children;
}
static inline size_t zend_ast_list_size(uint32_t children) {
	return sizeof(zend_ast_list) - sizeof(zend_ast *) + sizeof(zend_ast *) * children;
}

void ast_create_object(zval* return_value, zend_ast* ast, ast_tree* tree, zend_bool zval_as_value) {
	zval* cached_obj;
	zend_class_entry* ce = NULL;

	if (!ast) {
		RETURN_NULL();
	}

	cached_obj=zend_hash_index_find(&ASTG(nodes), (zend_ulong)ast);
	if (cached_obj) {
		RETURN_ZVAL(cached_obj, 1, 0);
	}

	switch (ast->kind) {
	case ZEND_AST_ZVAL:
		if (zval_as_value) {			
			RETURN_ZVAL(zend_ast_get_zval(ast), 1, 0);
		}
		ce = ast_zval_ce;
		break;

#define AST(id)
#define AST_DECL(id) case ZEND_AST_##id: ce=ast_decl_ce;break;
#define AST_LIST(id) case ZEND_AST_##id: ce=ast_list_ce;break;
#define AST_CHILD(id, children) case ZEND_AST_##id: ce = ast_node_ce;break;
#include "ast_kinds.h"
#undef AST_CHILD
#undef AST_LIST
#undef AST_DECL
#undef AST
	default:
		php_error_docref(NULL, E_WARNING, "Unknown AST node kind: %d", (int)ast->kind);
		RETURN_NULL();
	}

	ast_object* ast_obj;
	object_init_ex(return_value, ce);
	ast_obj = AST_OBJ_P(return_value);
	ast_obj->node = ast;
	ast_obj->tree = tree;
	++tree->refcount;
	//TODO remove :TRACE("CACHING WITH HASH %I64d", (zend_ulong)ast);
	zend_hash_index_add(&ASTG(nodes), (zend_ulong)ast, return_value);

}

void ast_destroy(zend_ast* ast) {
	zval* node; // the node corresponding to the specified ast
	if (!ast) {
		return;
	}

	// TODO deal with cache

	if (ast->kind == ZEND_AST_ZVAL) {
		zend_ast_destroy(ast);
		return;
	}
	if (ast->kind == ZEND_AST_ZNODE) {
		php_error_docref(NULL, E_WARNING, "Encountered unexpected AST_ZNODE");
	}
	if (zend_ast_is_list(ast)) {
		zend_ast_list* list = zend_ast_get_list(ast);
		uint32_t i;
		for (i = 0; i < list->children; i++) {
			ast_destroy(list->child[i]);
		}
		list->children = 0;
		zend_ast_destroy((zend_ast*)list);
		return;
	}
	if (ast->kind < (1 << ZEND_AST_IS_LIST_SHIFT)) {
		uint32_t i;
		zend_ast_decl* decl= (zend_ast_decl*)ast;
		for (i = 0; i < 4; i++) {
			ast_destroy(decl->child[i]);
			decl->child[i] = NULL;
		}	
		return;
	}

	uint32_t i;
	uint32_t children = zend_ast_get_num_children(ast);
	for (i = 0; i < children; i++) {
		ast_destroy(ast->child[i]);
		ast->child[i] = NULL;
	}
	zend_ast_destroy(ast);
}

/* {{{ performs a complete copy of the specified ast*/
zend_ast* ast_deep_copy(zend_ast* ast) {
	const char * kind;
	if (ast == NULL) {
		return NULL;
	}
	switch (ast->kind) {
#define OLD_CONST CONST
#undef CONST
#define AST(id) case ZEND_AST_##id:kind="ZEND_AST_"#id;break;
#define AST_DECL(id) AST(id)
#define AST_LIST(id) AST(id)
#define AST_CHILD(id, children) AST(id)
#include "ast_kinds.h"
#undef AST_CHILD
#undef AST_LIST
#undef AST_DECL
#undef AST
#define CONST OLD_CONST
#undef OLD_CONST
	}
	TRACE("found %s on line %d", kind, ast->lineno);
	if (ast->kind == ZEND_AST_ZNODE) {
		php_error_docref(NULL, E_WARNING, "Encountered unexpected AST_ZNODE");
		return NULL;
	}
	if (ast->kind == ZEND_AST_ZVAL) {
		zend_ast_zval* new = zend_arena_alloc(&CG(ast_arena), sizeof(zend_ast_zval));
		*new = *(zend_ast_zval*)ast;
		zval_copy_ctor(&new->val);
		return (zend_ast*)new;
	}
	if (ast->kind < (1 << ZEND_AST_IS_LIST_SHIFT)) {
		uint32_t i;
		zend_ast_decl* decl = (zend_ast_decl*)ast;
		zend_ast_decl* new = zend_arena_alloc(&CG(ast_arena), sizeof(zend_ast_decl));
		*new = *decl;
		if (new->doc_comment) {
			zend_string_addref(new->doc_comment);
		}
		if (new->name) {
			zend_string_addref(new->name);
		}
		for (i = 0; i < 4; i++) {
			new->child[i] = ast_deep_copy(decl->child[i]);
		}
		return (zend_ast*)new;
	}
	if (zend_ast_is_list(ast)) {
		uint32_t i;
		zend_ast_list* list = zend_ast_get_list(ast);
		zend_ast_list* new = zend_arena_alloc(&CG(ast_arena), zend_ast_list_size(list->children));
		*new = *list;
		for (i = 0; i < new->children; i++) {
			new->child[i] = ast_deep_copy(list->child[i]);
		}
		return (zend_ast*)new;
	}

	uint32_t i;
	uint32_t children_cnt = zend_ast_get_num_children(ast);
	zend_ast* new= zend_arena_alloc(&CG(ast_arena), zend_ast_size(children_cnt));
	*new = *ast;
	for (i = 0; i < children_cnt; i++) {
		new->child[i] = ast_deep_copy(ast->child[i]);
	}
	return new;
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
	zend_string* source_zstr;
	zend_long options;
	zval source_zval;
	zend_lex_state original_lexical_state;
	ast_tree* tree;
	int parse_result;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|l", &source_zstr, &options) == FAILURE) {
		RETURN_NULL();
	}

	zend_save_lexical_state(&original_lexical_state);
	ZVAL_STR_COPY(&source_zval, source_zstr);
	if (zend_prepare_string_for_scanning(&source_zval, "Ast:parseString()") == FAILURE) {
		zend_restore_lexical_state(&original_lexical_state);
		RETURN_NULL();
	}

	CG(ast) = NULL;
	CG(ast_arena) = zend_arena_create(1024 * 32);
	parse_result = zendparse();
	zval_dtor(&source_zval);

	if (parse_result != 0) {
		zend_ast_destroy(CG(ast));
		zend_arena_destroy(CG(arena));
		CG(ast) = NULL;
		CG(arena) = NULL;
		zend_restore_lexical_state(&original_lexical_state);
		RETURN_NULL();
	}

	if (zend_ast_process && !(options&AST_NO_PROCESS)) {
		zend_ast_process(CG(ast));
	}

	tree = emalloc(sizeof(ast_tree));
	tree->root = CG(ast);
	tree->arena = CG(ast_arena);
	tree->refcount = 0;

	CG(ast) = NULL;
	CG(ast_arena) = NULL;
	zend_restore_lexical_state(&original_lexical_state);
	ast_create_object(return_value, tree->root, tree, 0);
}/* }}} */

 /* {{{ proto Ast::parseFile(string filename, int options) */
ZEND_BEGIN_ARG_INFO(ast_parsefile_arginfo, 0)
ZEND_ARG_INFO(0, filename)
ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()
static PHP_METHOD(Ast, parseFile) {
	zend_string* filename_zstr;
	zend_long options;
	zend_file_handle file_handle;
	zend_lex_state original_lexical_state;
	int parse_result;

	ast_tree* tree;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|l", &filename_zstr, &options)) {
		RETURN_NULL();
	}

	if (zend_stream_open(ZSTR_VAL(filename_zstr), &file_handle) == FAILURE) {
		php_error_docref(NULL, E_WARNING, "Failed to open file");
		RETURN_NULL();
	}
	if (open_file_for_scanning(&file_handle) == FAILURE) {
		zend_destroy_file_handle(&file_handle);
		RETURN_NULL();
	}

	CG(ast) = NULL;
	CG(ast_arena) = zend_arena_create(1024*32);
	zend_save_lexical_state(&original_lexical_state);
	
	parse_result = zendparse();
	
	if (parse_result != 0) {
		zend_ast_destroy(CG(ast));
		zend_arena_destroy(CG(ast_arena));
		zend_restore_lexical_state(&original_lexical_state);
		zend_destroy_file_handle(&file_handle);
		RETURN_NULL();
	}

	if (zend_ast_process && !(options&AST_NO_PROCESS)) {
		zend_ast_process(CG(ast));
	}

	tree = emalloc(sizeof(ast_tree));
	tree->root = CG(ast);
	tree->arena = CG(ast_arena);
	tree->refcount = 0;

	CG(ast) = NULL;
	CG(ast_arena) = NULL;
	zend_restore_lexical_state(&original_lexical_state);
	zend_destroy_file_handle(&file_handle);
	ast_create_object(return_value, tree->root, tree, 0);	
}/* }}} */

 /* {{{ proto Ast::getNode(string $code, int $options) */
ZEND_BEGIN_ARG_INFO(ast_getnode_arginfo, 0)
ZEND_ARG_TYPE_INFO(0, filename, IS_STRING, 0)
//ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()
static PHP_METHOD(Ast, getNode) {
	zend_string* filename_zstr;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &filename_zstr) == FAILURE) {
		RETURN_NULL();
	}

	zval* node_hash;
	zval* node;
	// TODO remove : TRACE("FILENAME HASH %I64d", ZSTR_HASH(filename_zstr));
	node_hash = zend_hash_index_find(&ASTG(files), ZSTR_HASH(filename_zstr));
	if (node_hash == NULL) {
		RETURN_NULL();
	}
	// TODO remove : TRACE("LOOKING UP NODE HASH %I64d", Z_LVAL_P(node_hash));
	node = zend_hash_index_find(&ASTG(nodes), Z_LVAL_P(node_hash));
	if (node == NULL) {
		RETURN_NULL();
	}
	ZVAL_COPY(return_value, node);
	

}
/* }}} */

static zend_function_entry ast_methods[] = {
	PHP_ME(Ast, __construct, NULL, ZEND_ACC_PRIVATE | ZEND_ACC_CTOR)
	PHP_ME(Ast, parseString, ast_parsestring_arginfo, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(Ast, parseFile, ast_parsefile_arginfo, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(Ast, getNode, ast_getnode_arginfo, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_FE_END
};

int ast_minit(INIT_FUNC_ARGS) {
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "Ast", ast_methods);
	ast_ce = zend_register_internal_class(&ce);
	return SUCCESS;
};

////////////////////////////////////////////////////////////
// AstNode class

static zend_object* ast_node_create(zend_class_entry* ce) {
	TRACE("creating ast node...");
	ast_object* astobj = ecalloc(1, sizeof(ast_object) + zend_object_properties_size(ce));
	zend_object* zobj = zend_object_from_ast_object(astobj);
	zend_object_std_init(zobj, ce);
	zobj->handlers = &ast_node_handlers;
	object_properties_init(zobj, ce);
	return zobj;
}

static zend_object* ast_node_clone(zval* zval_p) {
	TRACE("cloning ast node : TODO");
	return NULL;
}

static void ast_node_free(zend_object* zobj) {
	TRACE("freeing ast node");

	ast_object* astobj = ast_object_from_zend_object(zobj);
	//TODO remove from cache

	ast_destroy(astobj->node);

	if (astobj->tree) {
		if (--astobj->tree->refcount <= 0) {
			zend_ast_destroy(astobj->tree->root);
			zend_arena_destroy(astobj->tree->arena);
			efree(astobj->tree);
			astobj->tree = NULL;
		}
	}
	astobj = NULL;
	zend_object_std_dtor(zobj);
}

static PHP_METHOD(AstNode, export) {
	ast_object* this_astobj;
	this_astobj = AST_OBJ_P(getThis());
	zend_string* result_zstr = zend_ast_export("", this_astobj->node, "");
	if (result_zstr) {
		RETURN_STR(result_zstr);
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
	ast_node_ce = zend_register_internal_class(&ce);
	ast_node_ce->create_object = ast_node_create;



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
	ast_decl_ce = zend_register_internal_class_ex(&ce, ast_node_ce);
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
	ast_list_ce = zend_register_internal_class_ex(&ce, ast_node_ce);
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
	ast_zval_ce = zend_register_internal_class_ex(&ce, ast_node_ce);
	return SUCCESS;
}

////////////////////////////////////////////////////////////
//

static void ast_process(zend_ast* ast) {
	ZEND_ASSERT(ast == CG(ast));
	zend_lex_state original_lexical_state;
	zend_arena* original_ast_arena;
	ast_tree* tree;
	zval node_zval;

	long leakbytes = 3;
	emalloc(leakbytes);

	zend_save_lexical_state(&original_lexical_state);
	original_ast_arena = CG(ast_arena);
	CG(ast_arena) = zend_arena_create(1024 * 32);

	TRACE("processing ast for '%s'", ZSTR_VAL(original_lexical_state.filename));

	tree = emalloc(sizeof(ast_tree));
	tree->root = ast_deep_copy(ast);
	tree->arena = CG(ast_arena);
	tree->refcount = 0;

	ast_create_object(&node_zval, tree->root, tree, 1);

	zval* index;
	index = emalloc(sizeof(zval));
	
	ZVAL_LONG(index, (zend_ulong)tree->root);
	// TODO remove : TRACE("FILENAME HASH %I64d", ZSTR_HASH(original_lexical_state.filename));
	// TODO remove : TRACE("CACHING NODE HASH %I64d", Z_LVAL_P(index));
	zend_hash_index_add(&ASTG(files), ZSTR_HASH(original_lexical_state.filename), index);
	
	zval* node_hash;
	zval* node;

	node_hash = zend_hash_index_find(&ASTG(files), ZSTR_HASH(original_lexical_state.filename));
	
	// TODO remove : TRACE("LOOKING UP NODE HASH %I64d", Z_LVAL_P(node_hash));
	
	CG(ast_arena) = original_ast_arena;
	zend_restore_lexical_state(&original_lexical_state);
	
	if (prev_ast_process) {
		prev_ast_process(ast);
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
	zend_hash_init(&ast_globals->files, 32, NULL, NULL, 1);
}

#ifdef COMPILE_DL_AST
ZEND_GET_MODULE(ast)
#endif