
/* special nodes */
AST(ZVAL)
AST(ZNODE)

/* declaration nodes */
AST_DECL(FUNC_DECL)
AST_DECL(CLOSURE)
AST_DECL(METHOD)
AST_DECL(CLASS)

/* list nodes */
AST_LIST(ARG_LIST)
#if PHP_VERSION_ID < 70100
	AST_LIST(LIST)
#endif
	AST_LIST(ARRAY)
	AST_LIST(ENCAPS_LIST)
	AST_LIST(EXPR_LIST)
	AST_LIST(STMT_LIST)
	AST_LIST(IF)
	AST_LIST(SWITCH_LIST)
	AST_LIST(CATCH_LIST)
	AST_LIST(PARAM_LIST)
	AST_LIST(CLOSURE_USES)
	AST_LIST(PROP_DECL)
	AST_LIST(CONST_DECL)
	AST_LIST(CLASS_CONST_DECL)
	AST_LIST(NAME_LIST)
	AST_LIST(TRAIT_ADAPTATIONS)
	AST_LIST(USE)

	/* 0 child nodes */
	AST_CHILD(MAGIC_CONST, 0)
	AST_CHILD(TYPE, 0)

	/* 1 child node */
	AST_CHILD(VAR, 1)
	AST_CHILD(CONST, 1)
	AST_CHILD(UNPACK, 1)
	AST_CHILD(UNARY_PLUS, 1)
	AST_CHILD(UNARY_MINUS, 1)
	AST_CHILD(CAST, 1)
	AST_CHILD(EMPTY, 1)
	AST_CHILD(ISSET, 1)
	AST_CHILD(SILENCE, 1)
	AST_CHILD(SHELL_EXEC, 1)
	AST_CHILD(CLONE, 1)
	AST_CHILD(EXIT, 1)
	AST_CHILD(PRINT, 1)
	AST_CHILD(INCLUDE_OR_EVAL, 1)
	AST_CHILD(UNARY_OP, 1)
	AST_CHILD(PRE_INC, 1)
	AST_CHILD(PRE_DEC, 1)
	AST_CHILD(POST_INC, 1)
	AST_CHILD(POST_DEC, 1)
	AST_CHILD(YIELD_FROM, 1)

	AST_CHILD(GLOBAL, 1)
	AST_CHILD(UNSET, 1)
	AST_CHILD(RETURN, 1)
	AST_CHILD(LABEL, 1)
	AST_CHILD(REF, 1)
	AST_CHILD(HALT_COMPILER, 1)
	AST_CHILD(ECHO, 1)
	AST_CHILD(THROW, 1)
	AST_CHILD(GOTO, 1)
	AST_CHILD(BREAK, 1)
	AST_CHILD(CONTINUE, 1)

	/* 2 child nodes */
	AST_CHILD(DIM, 2)
	AST_CHILD(PROP, 2)
	AST_CHILD(STATIC_PROP, 2)
	AST_CHILD(CALL, 2)
	AST_CHILD(CLASS_CONST, 2)
	AST_CHILD(ASSIGN, 2)
	AST_CHILD(ASSIGN_REF, 2)
	AST_CHILD(ASSIGN_OP, 2)
	AST_CHILD(BINARY_OP, 2)
	AST_CHILD(GREATER, 2)
	AST_CHILD(GREATER_EQUAL, 2)
	AST_CHILD(AND, 2)
	AST_CHILD(OR, 2)
	AST_CHILD(ARRAY_ELEM, 2)
	AST_CHILD(NEW, 2)
	AST_CHILD(INSTANCEOF, 2)
	AST_CHILD(YIELD, 2)
	AST_CHILD(COALESCE, 2)

	AST_CHILD(STATIC, 2)
	AST_CHILD(WHILE, 2)
	AST_CHILD(DO_WHILE, 2)
	AST_CHILD(IF_ELEM, 2)
	AST_CHILD(SWITCH, 2)
	AST_CHILD(SWITCH_CASE, 2)
	AST_CHILD(DECLARE, 2)
#if PHP_VERSION_ID < 70100
	AST_CHILD(CONST_ELEM, 2)
#endif
	AST_CHILD(USE_TRAIT, 2)
	AST_CHILD(TRAIT_PRECEDENCE, 2)
	AST_CHILD(METHOD_REFERENCE, 2)
	AST_CHILD(NAMESPACE, 2)
	AST_CHILD(USE_ELEM, 2)
	AST_CHILD(TRAIT_ALIAS, 2)
	AST_CHILD(GROUP_USE, 2)

	/* 3 child nodes */
	AST_CHILD(METHOD_CALL, 3)
	AST_CHILD(STATIC_CALL, 3)
	AST_CHILD(CONDITIONAL, 3)

	AST_CHILD(TRY, 3)
	AST_CHILD(CATCH, 3)
	AST_CHILD(PARAM, 3)
	AST_CHILD(PROP_ELEM, 3)
#if PHP_VERSION_ID >= 70100
	AST_CHILD(CONST_ELEM, 3)
#endif

	/* 4 child nodes */
	AST_CHILD(FOR, 4)
	AST_CHILD(FOREACH, 4)
