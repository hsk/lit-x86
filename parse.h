#ifndef _PARSE_LIT_
#define _PARSE_LIT_

#include "common.h"
#include "var.h"
#include "token.h"
#include "func.h"
#include "var.h"
#include "ast.h"
#include "exprtype.h"

enum {
	ADDR_SIZE = 4
};

enum {
	BLOCK_NORMAL = 0,
	BLOCK_LOOP,
	BLOCK_FUNC,
	BLOCK_GLOBAL,
	NON
};

class Parser {
public:
	Token &tok;
	int blocksCount;
	std::string module;

	Parser(Token &token)
		:tok(token) { }

// expr.h
	int is_string_tok();
	int is_number_tok();
	int is_ident_tok();
	int is_char_tok();

	AST *expr_entry();
	AST *expr_asgmt();
	AST *expr_compare();
	AST *expr_logic();
	AST *expr_add_sub();
	AST *expr_mul_div();
	AST *expr_index();
	AST *expr_postfix();
	AST *expr_primary();
	AST *expr_array();

// parse.h
	AST *make_lib();
	AST *make_proto();
	AST *make_if();
	AST *make_elsif();
	AST *make_while();
	AST *make_for();
	AST *make_func();
	AST *make_break();
	AST *make_return();
	AST *make_struct();

	ast_vector eval();
	AST *expression();

	int parser();
	int get_string();

	std::map<std::string, bool> function_list;
	bool is_func(std::string);
	void append_func(std::string);
};

char *replace_escape(char *);

#endif
