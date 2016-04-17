#include "expr.h"
#include "lit.h"
#include "asm.h"
#include "lex.h"
#include "token.h"
#include "parse.h"
#include "util.h"
#include "var.h"
#include "func.h"
#include "ast.h"
#include "exprtype.h"

int Parser::is_string_tok() { return tok.get().type == TOK_STRING; }
int Parser::is_number_tok() { return tok.get().type == TOK_NUMBER; }
int Parser::is_ident_tok()  { return tok.get().type == TOK_IDENT;  }
int Parser::is_char_tok() { return tok.get().type == TOK_CHAR; } 

AST *visit(AST *ast) {
	if(ast->get_type() == AST_BINARY) {
		std::cout << "(" << ((BinaryAST *)ast)->op << " ";
			visit(((BinaryAST *)ast)->left);
			visit(((BinaryAST *)ast)->right);
		std::cout << ")";
		std::cout << std::endl;
	} else if(ast->get_type() == AST_VARIABLE_ASGMT) {
		std::cout << "(=";
			visit(((VariableAsgmtAST *)ast)->var);
			visit(((VariableAsgmtAST *)ast)->src);
		std::cout << ")" << std::endl;;
	} else if(ast->get_type() == AST_POSTFIX) {
		std::cout << "(" << ((PostfixAST *)ast)->op << " ";
			visit(((PostfixAST *)ast)->expr);
			std::cout << ")";
	} else if(ast->get_type() == AST_WHILE) {
		WhileAST *wa = (WhileAST *)ast;
		std::cout << "(while ("; visit(wa->cond);
		std::cout << ")\n(\n";
		for(int i = 0; i < wa->block.size(); i++) 
			visit(wa->block[i]);
		std::cout << ")\n)";
		std::cout << std::endl;
	} else if(ast->get_type() == AST_FOR) {
		ForAST *fa = (ForAST *)ast;
		std::cout << "(for ("; visit(fa->asgmt);
		if(fa->is_range_for) {
			std::cout << ") ("; visit(fa->range);
			std::cout << ")\n(" << std::endl;
		} else {
			std::cout << ") ("; visit(fa->cond); 
			std::cout << ") ("; visit(fa->step);
			std::cout << ")\n(" << std::endl;
		}
		for(int i = 0; i < fa->block.size(); i++) 
			visit(fa->block[i]);
		std::cout << ")\n)";
		std::cout << std::endl;
	} else if(ast->get_type() == AST_FUNCTION) {
		FunctionAST *fa = ((FunctionAST *) ast);
		std::cout << "(defunc " << fa->info.mod_name << "::" << fa->info.name << " ("; 
		for(int i = 0; i < fa->args.size(); i++) 
			visit(fa->args[i]);
		std::cout << ")\n(\n";
		for(int i = 0; i < fa->statement.size(); i++) 
			visit(fa->statement[i]);
		std::cout << ")\n)";
		std::cout << std::endl;
	} else if(ast->get_type() == AST_IF) {
		IfAST *ia = (IfAST *)ast;
		std::cout << "(if ("; visit(ia->cond); std::cout << ")\n(";
		for(int i = 0; i < ia->then_block.size(); i++) 
			visit(ia->then_block[i]);
		std::cout << ")\n(\n";
		for(int i = 0; i < ia->else_block.size(); i++) 
			visit(ia->else_block[i]);
		std::cout << ")\n)";
		std::cout << std::endl;
	} else if(ast->get_type() == AST_VARIABLE_INDEX) {
		std::cout << "([] ";
			visit(((VariableIndexAST *)ast)->var);
			visit(((VariableIndexAST *)ast)->idx);
		std::cout << ")";
	} else if(ast->get_type() == AST_VARIABLE_DECL) {
		std::cout << "(vardecl "
			<< ((VariableDeclAST *)ast)->info.mod_name << "::"
			<< ((VariableDeclAST *)ast)->info.name << " "
			<< ((VariableDeclAST *)ast)->info.type << ")";
	} else if(ast->get_type() == AST_ARRAY) {
		ArrayAST *ary = (ArrayAST *)ast;
		std::cout << "(array ";
		for(int i = 0; i < ary->elems.size(); i++) 
			visit(ary->elems[i]);
		std::cout << ")";
	} else if(ast->get_type() == AST_NUMBER) {
		std::cout << " " << ((NumberAST *)ast)->number << " ";
	} else if(ast->get_type() == AST_CHAR) {
		std::cout << " " << ((CharAST *)ast)->ch << " ";
	} else if(ast->get_type() == AST_STRING) {
		std::cout << " \"" << ((StringAST *)ast)->str << "\" ";
	} else if(ast->get_type() == AST_VARIABLE) {
		std::cout << "(var " 
			<< ((VariableAST *)ast)->info.mod_name << "::"
			<< ((VariableAST *)ast)->info.name << ") ";
	} else if(ast->get_type() == AST_FUNCTION_CALL) {
		std::cout << "(call " 
			<< ((FunctionCallAST *)ast)->info.mod_name << "::"
			<< ((FunctionCallAST *)ast)->info.name << " ";
		for(int i = 0; i < ((FunctionCallAST *)ast)->args.size(); i++) {
			visit(((FunctionCallAST *)ast)->args[i]);
		}
		std::cout << ")";
		std::cout << std::endl;
	} else if(ast->get_type() == AST_RETURN) {
		std::cout << "(return ("; visit(((ReturnAST *)ast)->expr);
		std::cout << "))" << std::endl;
	} else if(ast->get_type() == AST_LIBRARY) {
		LibraryAST *l = (LibraryAST *)ast;
		std::cout << "(lib " << l->lib_name << "(" << std::endl;
		for(int i = 0; i < l->proto.size(); i++) 
			visit(l->proto[i]);
		std::cout << ")" << std::endl;
	} else if(ast->get_type() == AST_PROTO) {
		PrototypeAST *prt = (PrototypeAST *)ast;
		std::cout << "(proto " << prt->proto.name << " " << prt->proto.params << ")" << std::endl;
	} else if(ast->get_type() == AST_NEW) {
		NewAllocAST *na = (NewAllocAST *)ast;
		std::cout << "(new " << na->type << " ";
		visit(na->size);
		std::cout << ")" << std::endl;
	}

	return ast;
}

AST *Parser::expr_entry() { 
	return expr_asgmt();
}

AST *Parser::expr_asgmt() {
	AST *l, *r;
	bool add = false, sub = false, mul = false, div = false;
	l = expr_compare();
	while((add = tok.skip("+=")) || (sub = tok.skip("-=")) || (mul = tok.skip("*=")) ||
			(div = tok.skip("/=")) || tok.skip("=")) {
		r = expr_entry();
		if(add || sub || mul || div) {
			l = new VariableAsgmtAST(l,
					new BinaryAST(add ? "+" : sub ? "-" : mul ? "*" : div ? "/" : "EXCEPTION", l, r));	
		} else 
			l = new VariableAsgmtAST(l, r);
	}
	return l;
}

AST *Parser::expr_compare() {
	bool andop=0, orop=0, xorop = false;
	AST *l, *r;
	l = expr_logic();
	while((andop=tok.skip("and") || tok.skip("&")) || (orop=tok.skip("or") || 
				tok.skip("|")) || (xorop=(tok.skip("xor") || tok.skip("^"))) || tok.skip("..")) {
		r = expr_logic();
		l = new BinaryAST(andop ? "and" : orop ? "or" : xorop ? "xor" : "range", l, r);
	}

	return l;
}

AST *Parser::expr_logic() {
	bool lt = false, gt=false, ne=false, eql=false, fle=false;
	AST *l, *r;
	l = expr_add_sub();
	if((lt=tok.skip("<")) || (gt=tok.skip(">")) || (ne=tok.skip("!=")) ||
			(eql=tok.skip("==")) || (fle=tok.skip("<=")) || tok.skip(">=")) {
		r = expr_add_sub();
		l = new BinaryAST(lt ? "<" : gt ? ">" : ne ? "!=" : eql ? "==" : fle ? "<=" : ">=", l, r);
	}

	return l;
}

AST *Parser::expr_add_sub() {
	int add = 0, concat = 0;
	AST *l, *r;
	l = expr_mul_div();
	while((add = tok.skip("+")) || (concat = tok.skip("~")) || tok.skip("-")) {
		r = expr_mul_div();
		l = new BinaryAST(add ? "+" : concat ? "~" : "-", l, r);
	}
	return l;
}

AST *Parser::expr_mul_div() {
	int mul, div;
	AST *l, *r;
	l = expr_index();
	while((mul = tok.skip("*")) || (div=tok.skip("/")) || tok.skip("%")) {
		r = expr_index();
		l = new BinaryAST(mul ? "*" : div ? "/" : "%", l, r);
	}
	return l;
}

AST *Parser::expr_index() {
	AST *l, *r;
	l = expr_postfix();
	while(tok.skip("[")) {
		r = expr_entry();
		l = new VariableIndexAST(l, r);
		if(!tok.skip("]"))
			error("error: %d: expected expression ']'", tok.get().nline);
	}
	return l;
}

AST *Parser::expr_postfix() {
	AST *expr = expr_primary();
	bool inc;
	if((inc = tok.skip("++")) || tok.skip("--")) {
		return new PostfixAST(inc ? "++" : "--", expr);
	}
	return expr;
}

AST *Parser::expr_primary() {
	bool is_get_addr = false, ispare = false, is_global_decl = false;
	std::string name, mod_name = "";
	var_t *v = NULL; 
	
	if(tok.skip("&")) is_get_addr = true;
	if(tok.skip("$")) is_global_decl = true;

	if(tok.get().val == "new") {
		tok.skip();
		AST *size = expr_entry();
		std::string type = "int";
		if(is_ident_tok())
			type = tok.next().val;
		return new NewAllocAST(type, size);
	} else if(is_number_tok()) {
		return new NumberAST(atoi(tok.next().val.c_str()));
	} else if(is_char_tok()) { 
		return new CharAST(tok.next().val.c_str()[0]);
	} else if(is_string_tok()) {
		return new StringAST(tok.next().val);
	} else if(tok.get().val == "true" || tok.get().val == "false") {
		return new NumberAST(tok.next().val == "true" ? 1 : 0);
	} else if(is_ident_tok()) { // variable or inc or dec
		name = tok.next().val; mod_name = "";
		int type, is_ary; 
		bool is_vardecl = false; 
		std::string class_name;

		if(tok.skip("::")) { // module?
			mod_name = tok.next().val;
			swap(mod_name, name);
		} else if(tok.skip(":")) { // variable declaration
			is_ary = 0;
			type = Type::str_to_type(tok.next().val);
			if(tok.skip("[]")) type |= T_ARRAY;
			is_vardecl = true;
		} else { 
			type = T_INT;
		}
		
		{	
			bool has_pare = false;
			if((has_pare=tok.skip("(")) || is_func(name)) { // function
				func_t f = {
					.name = name,
					// .mod_name = mod_name == "" ? module : mod_name
				};
				std::vector<AST *> args;
				if(HAS_PARAMS_FUNC) {
					while(!tok.is(")") && !tok.is(";")) {
						args.push_back(expr_entry());
						tok.skip(",");
					} 				
				} if(has_pare) tok.skip(")");
				return new FunctionCallAST(f, args);
			} else { // variable
				var_t v = {
					.name = name,
					.mod_name = mod_name == "" ? module : mod_name,
					.type = type,
					.class_type = class_name,
					.is_global = is_global_decl
				};
				if(is_vardecl)
					return new VariableDeclAST(v);
				else
					return new VariableAST(v);
			}
		}
	} else if(tok.skip("(")) {
		AST *e = expr_asgmt();
		if(!tok.skip(")"))
			error("error: %d: expected expression ')'", tok.get().nline);
		return e;
	} else {
		AST *ary = expr_array();
		if(ary == NULL)
			error("error: %d: invalid expression", tok.get().nline);
		else return ary;
	}

	return 0;
}

AST *Parser::expr_array() {
	if(tok.skip("[")) {
		ast_vector elems;
		while(!tok.skip("]")) {
			AST *elem = expr_asgmt();
			elems.push_back(elem);
			tok.skip(",");
		}
		return new ArrayAST(elems);
	}
	return NULL;
}

