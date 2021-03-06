#ifndef _AST_LIT_
#define _AST_LIT_

#include "common.h"
#include "var.h"
#include "func.h"
#include "asm.h"

enum {
	AST_NUMBER,
	AST_STRING,
	AST_CHAR,
	AST_POSTFIX,
	AST_BINARY,
	AST_VARIABLE,
	AST_VARIABLE_DECL,
	AST_VARIABLE_ASGMT,
	AST_VARIABLE_INDEX,
	AST_FUNCTION_CALL,
	AST_FUNCTION,
	AST_IF,
	AST_WHILE,
	AST_FOR,
	AST_BREAK,
	AST_RETURN,
	AST_ARRAY,
	AST_LIBRARY,
	AST_PROTO,
	AST_NEW,
	AST_STRUCT,
};

class AST {
public:
	virtual int get_type() const = 0;
	virtual ~AST() {};
};

class NumberAST : public AST {
public:
	int32_t number;
	NumberAST(int);
	virtual int get_type() const { return AST_NUMBER; };
	void codegen(Function &, NativeCode_x86 &);
};

class CharAST : public AST {
public:
	int ch;
	CharAST(int);
	virtual int get_type() const { return  AST_CHAR; }
	void codegen(Function &, NativeCode_x86 &);
};

class StringAST : public AST {
public:
	std::string str;
	StringAST(std::string);
	virtual int get_type() const { return AST_STRING; };
	void codegen(Function &, NativeCode_x86 &);
};

class PostfixAST : public AST {
public:
	std::string op;
	AST *expr;
	PostfixAST(std::string, AST *);
	~PostfixAST() { delete expr; }
	virtual int get_type() const { return AST_POSTFIX; }
};

class BinaryAST : public AST {
public:
	std::string op;
	AST *left, *right;
	BinaryAST(std::string o, AST *le, AST *re);
	virtual int get_type() const { return AST_BINARY; }
	int codegen(Function &, Module &, NativeCode_x86 &); // ret is type of expr.
};

class NewAllocAST : public AST {
public:
	std::string type;
	AST * size;
	NewAllocAST(std::string, AST *size);
	virtual int get_type() const { return AST_NEW; }
	int codegen(Function &, Module &, NativeCode_x86 &);
};

class VariableAST : public AST {
public:
	var_t info;
	VariableAST(var_t v);
	virtual int get_type() const { return AST_VARIABLE; }
	void codegen(Function &, Module &, NativeCode_x86 &);
	var_t *get(Function &, Module &);
	var_t *append(Function &, Module &);
};

class VariableDeclAST : public AST {
public:
	var_t info;
	VariableDeclAST(var_t);
	virtual int get_type() const { return AST_VARIABLE_DECL; }
	var_t *get(Function &, Module &);
	var_t *append(Function &, Module &);
};

class VariableAsgmtAST : public AST {
public:
	AST *var, *src;
	VariableAsgmtAST(AST *, AST *);
	virtual int get_type() const { return AST_VARIABLE_ASGMT; }
	void codegen(Function &, Module &, NativeCode_x86 &);
};

class VariableIndexAST : public AST {
public:
	AST *var, *idx;
	VariableIndexAST(AST *, AST *);
	virtual int get_type() const { return AST_VARIABLE_INDEX; }
	int codegen(Function &, Module &, NativeCode_x86 &);
};

class LibraryAST : public AST {
public:
	std::string lib_name;
	std::vector<AST *> proto;
	LibraryAST(std::string, std::vector<AST *>);
	virtual int get_type() const { return AST_LIBRARY; }
	int codegen(Module &, NativeCode_x86 &);
};

class PrototypeAST : public AST {
public:
	func_t proto;
	std::vector<AST *> args_type;
	PrototypeAST(func_t, std::vector<AST *>);	
	virtual int get_type() const { return AST_PROTO; }
	void append(void *, Module &);
};

class FunctionCallAST : public AST {
public:
	func_t info;
	std::vector<AST *> args;
	FunctionCallAST(func_t f, std::vector<AST *> a);
	virtual int get_type() const { return AST_FUNCTION_CALL; }
	int codegen(Function &, Module &, NativeCode_x86 &);
};

class FunctionAST : public AST {
public:
	func_t info;
	std::vector<AST *> args, statement;
	FunctionAST(func_t f, std::vector<AST *> a, std::vector<AST *>);
	virtual int get_type() const { return AST_FUNCTION; }
	Function codegen(Module &);
};

class StructAST : public AST {
public:
	std::string name;
	std::vector<AST *> var_decls;
	StructAST(std::string, std::vector<AST *>);
	virtual int get_type() const { return AST_STRUCT; }
};

class ArrayAST : public AST {
public:
	std::vector<AST *> elems;
	ArrayAST(std::vector<AST *>);
	virtual int get_type() const { return AST_ARRAY; }
	int codegen(Function &, Module &, NativeCode_x86 &);
};

class IfAST : public AST {
public:
	AST *cond;
	std::vector<AST *> then_block, else_block;
	IfAST(AST *, std::vector<AST*>, std::vector<AST *>);
	virtual int get_type() const { return AST_IF; }
	void codegen(Function &, Module &, NativeCode_x86 &);
};

class WhileAST : public AST {
public:
	AST *cond;
	std::vector<AST *> block;
	WhileAST(AST *, std::vector<AST *>);
	virtual int get_type() const { return AST_WHILE; }
	void codegen(Function &, Module &, NativeCode_x86 &);
};

class ForAST : public AST {
public:
	bool is_range_for;
	AST *asgmt, *cond, *step, *range;
	std::vector<AST *> block;
	ForAST(AST *, AST *, AST *, std::vector<AST *>); // assignment, condition, step, body
	virtual int get_type() const { return AST_FOR; }
	void codegen(Function &, Module &, NativeCode_x86 &);
};

class BreakAST : public AST {
public:
	virtual int get_type() const { return AST_BREAK; }
	void codegen(Function &, Module &, NativeCode_x86 &);
};

class ReturnAST : public AST {
public:
	AST *expr;
	ReturnAST(AST *);
	virtual int get_type() const { return AST_RETURN; }
	void codegen(Function &, Module &, NativeCode_x86 &);
};

typedef std::vector<AST *> ast_vector;


#endif
