#include "php_ast.h"

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
	return SUCCESS;
};

PHP_MSHUTDOWN_FUNCTION(ast) {	
	return SUCCESS;
};


PHP_GINIT_FUNCTION(ast) {
#if defined(COMPILE_DL_AST) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif	
}

#ifdef COMPILE_DL_AST
ZEND_GET_MODULE(ast)
#endif