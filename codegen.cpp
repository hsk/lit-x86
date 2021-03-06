#include "codegen.h"
#include "lit.h"
#include "asm.h"
#include "expr.h"
#include "exprtype.h"
#include "util.h"
#include "parse.h"

int codegen_entry(ast_vector &program) {
	uint32_t main_address;
	std::string module = "";
	ntv.gencode(0xe9); main_address = ntv.count; ntv.gencode_int32(0);
	Module list(module);

	Function main;
	std::vector<AST *> main_code;
	for(ast_vector::iterator it = program.begin(); it != program.end(); ++it) {
		if((*it)->get_type() == AST_FUNCTION) {
			Function f = ((FunctionAST *)*it)->codegen(list);
		} else if((*it)->get_type() == AST_LIBRARY) {
			((LibraryAST *)*it)->codegen(list, ntv);
		} else {
			main_code.push_back(*it);
		}
	}

	main.info.name = "main";
	main.info.address = ntv.count;
	ntv.genas("push ebp");
	ntv.genas("mov ebp esp");
	ntv.gencode(0x8b); ntv.gencode(0x75); ntv.gencode(0x0c); // mov esi, 0xc(%ebp)
	uint32_t esp_ = ntv.count + 2; ntv.genas("sub esp 0");
	
	list.append(main);
	for(std::vector<AST *>::iterator it = main_code.begin(); it != main_code.end(); ++it) {
		codegen_expression(main, list, *it);
	}

	int margin = 6;
	ntv.gencode_int32_insert(main.var.total_size() + ADDR_SIZE * margin, esp_);
	ntv.genas("add esp %u", main.var.total_size() + margin * ADDR_SIZE); // add esp nn
	ntv.gencode(0xc9);// leave
	ntv.gencode(0xc3);// ret

	list.insert_global_var();

	Function *_main = list.get("main");
		if(_main == NULL) error("error: not found function: 'main'");
	ntv.gencode_int32_insert(_main->info.address - ADDR_SIZE - 1, main_address);
	
	return 0;
}

bool const_folding(AST *e, int *res) { // TODO: rewrite more beautiful!
	BinaryAST *expr = (BinaryAST *)e;
	int tmp = 0;
	int a, b;

	if(expr->left->get_type() == AST_BINARY) {
		if(!const_folding(expr->left, &tmp))
			return false;
		a = tmp;
	} else if(expr->left->get_type() == AST_NUMBER) 
		a = ((NumberAST *)expr->left)->number;
	else return false;

	if(expr->right->get_type() == AST_BINARY) {
		if(!const_folding(expr->right, &tmp))
			return false;
		b = tmp;
	} else if(expr->right->get_type() == AST_NUMBER) 
		b = ((NumberAST *)expr->right)->number;
	else return false;

	if(expr->op == "+") *res = a + b;
	else if(expr->op == "-") *res = a - b;
	else if(expr->op == "*") *res = a * b;
	else if(expr->op == "/") *res = a / b;
	else if(expr->op == "%") *res = a % b;
	else return false;

	return true;
}

int codegen_expression(Function &f, Module &f_list, AST *ast) {
	switch(ast->get_type()) {
	case AST_NUMBER:
		((NumberAST *)ast)->codegen(f, ntv);
		return T_INT;
	case AST_STRING:
		((StringAST *)ast)->codegen(f, ntv);
		return T_STRING;
	case AST_CHAR:
		((CharAST *)ast)->codegen(f, ntv);
		return T_CHAR;
	case AST_NEW:
		return ((NewAllocAST *)ast)->codegen(f, f_list, ntv);
	case AST_VARIABLE:
		((VariableAST *)ast)->codegen(f, f_list, ntv);
		return ((VariableAST *)ast)->get(f, f_list)->type;
	case AST_VARIABLE_ASGMT:
		((VariableAsgmtAST *)ast)->codegen(f, f_list, ntv);
		return T_VOID;
	case AST_FUNCTION_CALL:
		return ((FunctionCallAST *)ast)->codegen(f, f_list, ntv);
	case AST_BINARY: 
		return ((BinaryAST *)ast)->codegen(f, f_list, ntv);
	case AST_ARRAY:
		return ((ArrayAST *)ast)->codegen(f, f_list, ntv);
	case AST_VARIABLE_INDEX:
		return ((VariableIndexAST *)ast)->codegen(f, f_list, ntv);
	case AST_IF:
		((IfAST *)ast)->codegen(f, f_list, ntv);
		return T_VOID;
	case AST_WHILE:
		((WhileAST *)ast)->codegen(f, f_list, ntv);
		return T_VOID;
	case AST_FOR:
		((ForAST *)ast)->codegen(f, f_list, ntv);
		return T_VOID;
	case AST_BREAK:
		((BreakAST *)ast)->codegen(f, f_list, ntv);
		return T_VOID;
	case AST_RETURN:
		((ReturnAST *)ast)->codegen(f, f_list, ntv);
		return T_VOID;
	}
	return T_VOID;
}

int LibraryAST::codegen(Module &f_list, NativeCode_x86 &ntv) {
	void *lib = dlopen(("./lib/" + lib_name + ".so").c_str(), RTLD_LAZY | RTLD_NOW);
	if(lib == NULL) error("LitSystemError: cannot load library '%s'", lib_name.c_str());
	/*
		lib_t l = {
			.name = name,
			.no = lib.size(),
			.handle = dlopen(("./lib/" + name + ".so").c_str(), RTLD_LAZY | RTLD_NOW)
		};
		if(l.handle == NULL)
	 */
	for(ast_vector::iterator it = proto.begin(); it != proto.end(); ++it) {
		((PrototypeAST *)*it)->append(lib, f_list);
	}
	return 0;
}

void PrototypeAST::append(void *lib, Module &f_list) {
	Function f;
	f.info.name = proto.name;
	f.info.mod_name = "";
	f.info.is_lib = true;
	f.info.address = (uint32_t)dlsym(lib, proto.name.c_str());
	f.info.params = args_type.size();
	f.info.type = proto.type;
	f_list.append(f);
}

Function FunctionAST::codegen(Module &f_list) {
	Function f;
	f.info.name = info.name;
	f.info.mod_name = "";
	f.info.address = ntv.count;
	f.info.params = args.size();
	f.info.type = info.type;
	uint32_t func_bgn = ntv.count;

	ntv.genas("push ebp");
	ntv.genas("mov ebp esp");
	uint32_t esp_ = ntv.count + 2; ntv.genas("sub esp 0");
	if(info.name == "main") {
		ntv.gencode(0x8b); ntv.gencode(0x75); ntv.gencode(0x0c); // mov esi, 0xc(%ebp)
	}

	// append arguments 
	for(ast_vector::iterator it = args.begin(); it != args.end(); ++it) {
		if((*it)->get_type() == AST_VARIABLE) {
			var_t *v = ((VariableAST *)*it)->append(f, f_list);
		} else if((*it)->get_type() == AST_VARIABLE_DECL) {
			((VariableDeclAST *)*it)->append(f, f_list);
			var_t *v = &((VariableDeclAST *)*it)->info;
			if(v->type & T_ARRAY || v->type == T_STRING) {
				ntv.gencode(0x8d); ntv.gencode(0x45);
					ntv.gencode(256 - ADDR_SIZE * v->id); // lea eax [esp - id*ADDR_SIZE]
				ntv.gencode(0x89); ntv.gencode(0x04); ntv.gencode(0x24); // mov [esp], eax
				ntv.gencode(0xff); ntv.gencode(0x56); ntv.gencode(44);// call *44(esi) LitMemory::append_ptr
			}
		}
	}

	f_list.append(f);
	f_list.rep_undef(f.info.name, func_bgn);

	for(ast_vector::iterator it = statement.begin(); it != statement.end(); ++it) { // function body
		codegen_expression(f, f_list, *it);
	}

	// generate code for 'return'
	for(std::vector<int>::iterator it = f.return_list.begin(); it != f.return_list.end(); ++it) {
		ntv.gencode_int32_insert(ntv.count - *it - ADDR_SIZE, *it);
	}

	int margin = 6;
	ntv.genas("add esp %u", f.var.total_size() + margin * ADDR_SIZE); // add esp nn
	ntv.gencode(0xc9);// leave
	ntv.gencode(0xc3);// ret

	ntv.gencode_int32_insert(f.var.total_size() + ADDR_SIZE * margin, esp_);
	return f;
}

void IfAST::codegen(Function &f, Module &f_list, NativeCode_x86 &ntv) {
	codegen_expression(f, f_list, cond);
	ntv.gencode(0x83); ntv.gencode(0xf8); ntv.gencode(0x00);// cmp eax, 0
	ntv.gencode(0x75); ntv.gencode(0x05); // jne 5
	ntv.gencode(0xe9); int addr_before_cond = ntv.count; ntv.gencode_int32(0);// jmp
	for(ast_vector::iterator it = then_block.begin(); it != then_block.end(); ++it)
		codegen_expression(f, f_list, *it);
	ntv.gencode(0xe9); int addr_end = ntv.count; ntv.gencode_int32(0);// jmp end
	ntv.gencode_int32_insert(ntv.count - addr_before_cond - 4, addr_before_cond);

	for(ast_vector::iterator it = else_block.begin(); it != else_block.end(); ++it)
		codegen_expression(f, f_list, *it);
	ntv.gencode_int32_insert(ntv.count - addr_end - 4, addr_end);
}

void WhileAST::codegen(Function &f, Module &f_list, NativeCode_x86 &ntv) {
	ntv.gencode(0x90); int loop_bgn = ntv.count;
		std::vector<int> *break_list = new std::vector<int>;
		f.break_list.push(break_list);
	codegen_expression(f, f_list, cond);
	ntv.gencode(0x83); ntv.gencode(0xf8); ntv.gencode(0x00);// cmp eax, 0
	ntv.gencode(0x75); ntv.gencode(0x05); // jne 5
	ntv.gencode(0xe9); int end = ntv.count; ntv.gencode_int32(0);// jmp while end

	for(ast_vector::iterator it = block.begin(); it != block.end(); ++it) {
		codegen_expression(f, f_list, *it);
	}

	ntv.gencode(0xe9); ntv.gencode_int32(0xFFFFFFFF - ntv.count + loop_bgn - ADDR_SIZE); // jmp n
	ntv.gencode_int32_insert(ntv.count - end - ADDR_SIZE, end);

	for(std::vector<int>::iterator it = f.break_list.top()->begin(); it != f.break_list.top()->end(); ++it) {
		ntv.gencode_int32_insert(ntv.count - *it - ADDR_SIZE, *it);
	}
	f.break_list.pop();
}

void ForAST::codegen(Function &f, Module &f_list, NativeCode_x86 &ntv) {
	codegen_expression(f, f_list, asgmt);
	ntv.gencode(0x90); int loop_bgn = ntv.count;
	std::vector<int> *break_list = new std::vector<int>;
	f.break_list.push(break_list);
	codegen_expression(f, f_list, cond);
	ntv.gencode(0x83); ntv.gencode(0xf8); ntv.gencode(0x00);// cmp eax, 0
	ntv.gencode(0x75); ntv.gencode(0x05); // jne 5
	ntv.gencode(0xe9); int end = ntv.count; ntv.gencode_int32(0x00000000);// jmp while end

	for(ast_vector::iterator it = block.begin(); it != block.end(); ++it) {
		codegen_expression(f, f_list, *it);
	}
	codegen_expression(f, f_list, step);

	ntv.gencode(0xe9); ntv.gencode_int32(0xFFFFFFFF - ntv.count + loop_bgn - ADDR_SIZE); // jmp n
	ntv.gencode_int32_insert(ntv.count - end - ADDR_SIZE, end);
	ntv.gencode(0x90); ntv.gencode(0x90);
	for(std::vector<int>::iterator it = f.break_list.top()->begin(); it != f.break_list.top()->end(); ++it) {
		ntv.gencode_int32_insert(ntv.count - *it - ADDR_SIZE, *it);
	}
}

int FunctionCallAST::codegen(Function &f, Module &f_list, NativeCode_x86 &ntv) {
	for(int i = 0; i < sizeof(stdfunc) / sizeof(stdfunc[0]); i++) {
		if(stdfunc[i].name == info.name) {
			if(info.name == "Array") {
				codegen_expression(f, f_list, args[0]);
				ntv.gencode(0x89); ntv.gencode(0x44); ntv.gencode(0x24); ntv.gencode(0 * ADDR_SIZE); // mov [esp+arg*ADDR_SIZE], eax
				ntv.genas("mov eax 4");
				ntv.gencode(0x89); ntv.gencode(0x44); ntv.gencode(0x24); ntv.gencode(1 * ADDR_SIZE); // mov [esp+arg*ADDR_SIZE], eax
				ntv.gencode(0xff); ntv.gencode(0x56); ntv.gencode(12); // call malloc
			} else if(info.name == "puts") {
				for(int i = 0; i < args.size(); i++) {
					int ty = codegen_expression(f, f_list, args[i]);
					ntv.gencode(0x89); ntv.gencode(0x04); ntv.gencode(0x24); // mov [esp], eax
					if(ty == T_STRING) {
						ntv.gencode(0xff); ntv.gencode(0x56); ntv.gencode(4);// call *4(esi) putString
					} else if(ty == T_CHAR) {
						ntv.gencode(0xff); ntv.gencode(0x56); ntv.gencode(60);// call *60(esi) putc
					} else {
						ntv.gencode(0xff); ntv.gencode(0x16); // call (esi) putNumber
					}
				}
				ntv.gencode(0xff); ntv.gencode(0x56); ntv.gencode(8);// call *0x08(esi) putLN
			} else {
				if(stdfunc[i].args == -1) { // vector
					uint32_t a = 0;
					for(ast_vector::iterator it = args.begin(); it != args.end(); ++it) {
						codegen_expression(f, f_list, *it);
						ntv.gencode(0x89); ntv.gencode(0x44); ntv.gencode(0x24); ntv.gencode(ADDR_SIZE * a++); // mov [esp+a*ADDR_SIZE], eax
					}
				} else { // normal function
					for(int arg = 0; arg < stdfunc[i].args; arg++) {
						codegen_expression(f, f_list, args[arg]);
						ntv.gencode(0x89); ntv.gencode(0x44); ntv.gencode(0x24); ntv.gencode(arg * ADDR_SIZE); // mov [esp+arg*ADDR_SIZE], eax
					}
				}
				ntv.gencode(0xff); ntv.gencode(0x56); ntv.gencode(stdfunc[i].addr); // call $function
			}
			return stdfunc[i].type;
		}
	}
	Function *function = f_list.get(info.name, info.mod_name);
	if(function == NULL) { // undefined
		uint32_t a = 3;
		for(ast_vector::iterator it = args.begin(); it != args.end(); ++it) {
			codegen_expression(f, f_list, *it);
			ntv.gencode(0x89); ntv.gencode(0x44); ntv.gencode(0x24); ntv.gencode(256 - a++ * ADDR_SIZE); // mov [esp+ADDR*a], eax
		}
		ntv.gencode(0xe8); f_list.append_undef(info.name, info.mod_name, ntv.count);
			ntv.gencode_int32(0x00000000); // call function
		return T_INT;
	} else { // defined
		if(args.size() != function->info.params) error("error: the number of arguments is not same");
		if(function->info.is_lib) {
			uint32_t a = 0;
			for(ast_vector::iterator it = args.begin(); it != args.end(); ++it) {
				codegen_expression(f, f_list, *it);
				ntv.gencode(0x89); ntv.gencode(0x44); ntv.gencode(0x24); ntv.gencode(a++ * ADDR_SIZE); // mov [esp+ADDR*a], eax
			}
		} else {
			uint32_t a = 3;
			for(ast_vector::iterator it = args.begin(); it != args.end(); ++it) {
				codegen_expression(f, f_list, *it);
				ntv.gencode(0x89); ntv.gencode(0x44); ntv.gencode(0x24); ntv.gencode(256 - a++ * ADDR_SIZE); // mov [esp-ADDR*a], eax
			}
		}
		function->call(ntv);
		return function->info.type;
	}
}

int BinaryAST::codegen(Function &f, Module &f_list, NativeCode_x86 &ntv) {
	int ty1 = codegen_expression(f, f_list, left), ty2 = T_VOID;
	int res; if(const_folding(this, &res)) {
		ntv.genas("mov eax %d", res);
		return T_INT;
	}
	if(right->get_type() == AST_NUMBER) {
		NumberAST *n = (NumberAST *)right;
		ntv.genas("mov ebx %d", n->number);
		ty2 = T_INT;
	} else {
		ntv.genas("push eax");
		ty2 = codegen_expression(f, f_list, right);
		ntv.genas("mov ebx eax");
		ntv.genas("pop eax");
	}
	if(ty1 != ty2) if(op != "+") error("error: type error"); // except string concat
	if(op == "+") {
		if(ty1 == T_STRING && ty2 == T_STRING) { // string + string
			ntv.gencode(0x89); ntv.gencode(0x44); ntv.gencode(0x24); ntv.gencode(ADDR_SIZE * 0); // mov [esp+0*ADDR_SIZE], eax
			ntv.gencode(0x89); ntv.gencode(0x5c); ntv.gencode(0x24); ntv.gencode(ADDR_SIZE * 1); // mov [esp+1*ADDR_SIZE], ebx
			ntv.gencode(0xff); ntv.gencode(0x56); ntv.gencode(56); // call rea_concat
		} else if(ty1 == T_STRING && ty2 == T_CHAR) { // string + char(int)
			ntv.gencode(0x89); ntv.gencode(0x44); ntv.gencode(0x24); ntv.gencode(ADDR_SIZE * 0); // mov [esp+0*ADDR_SIZE], eax
			ntv.gencode(0x89); ntv.gencode(0x5c); ntv.gencode(0x24); ntv.gencode(ADDR_SIZE * 1); // mov [esp+1*ADDR_SIZE], ebx
			ntv.gencode(0xff); ntv.gencode(0x56); ntv.gencode(76); // call rea_concat_char
		} else
			ntv.genas("add eax ebx");
	} else if(op == "-") ntv.genas("sub eax ebx");
	else if(op == "*") ntv.genas("mul ebx");
	else if(op == "/") {
		ntv.genas("mov edx 0");
		ntv.genas("div ebx");
	} else if(op == "%") {
		ntv.genas("mov edx 0");
		ntv.genas("div ebx");
		ntv.genas("mov eax edx");
	} else if(op == "<" || op == ">" || op == "!=" ||
			op == "==" || op == "<=" || op == ">=") {
		bool lt = op == "<", gt = op == ">", ne = op == "!=", eql = op == "==", fle = op == "<=";
		bool str_cmp = false;
		if(ne || eql) {
			if(ty1 == T_STRING && ty2 == T_STRING) { // string == or != string
				ntv.gencode(0x89); ntv.gencode(0x44); ntv.gencode(0x24); ntv.gencode(ADDR_SIZE * 0); // mov [esp+0*ADDR_SIZE], eax
				ntv.gencode(0x89); ntv.gencode(0x5c); ntv.gencode(0x24); ntv.gencode(ADDR_SIZE * 1); // mov [esp+1*ADDR_SIZE], ebx
				ntv.gencode(0xff); ntv.gencode(0x56); ntv.gencode(eql ? 80 : 84); // call streql or strne
				str_cmp = true;
			}
		} 
		if(!str_cmp) {
			ntv.gencode(0x39); ntv.gencode(0xd8); // cmp %eax, %ebx
			ntv.gencode(0x0f); ntv.gencode(lt ? 0x9c : gt ? 0x9f : ne ? 0x95 : eql ? 0x94 : fle ? 0x9e : 0x9d); ntv.gencode(0xc0); // setX al
			ntv.gencode(0x0f); ntv.gencode(0xb6); ntv.gencode(0xc0); // movzx eax al
		}
	} else if(op == "and" || op == "&" || op == "or" ||
			op == "|" || op == "xor" || op == "^") {
		bool andop = op == "and" || op == "&", orop = op == "or" || op == "|";
		ntv.gencode(andop ? 0x21 : orop ? 0x09 : 0x31); ntv.gencode(0xd8); // and eax ebx
	}
	return ty1;
}

int NewAllocAST::codegen(Function &f, Module &f_list, NativeCode_x86 &ntv) {
	int alloc_type = Type::str_to_type(type);
	codegen_expression(f, f_list, size);
	ntv.gencode(0x89); ntv.gencode(0x44); ntv.gencode(0x24); ntv.gencode(0 * ADDR_SIZE); // mov [esp], eax
	ntv.genas("mov eax 4");
	ntv.gencode(0x89); ntv.gencode(0x44); ntv.gencode(0x24); ntv.gencode(1 * ADDR_SIZE); // mov [esp+ADDR_SIZE], eax
	ntv.gencode(0xff); ntv.gencode(0x56); ntv.gencode(12); // call malloc
	return alloc_type | T_ARRAY;
}

void VariableAsgmtAST::codegen(Function &f, Module &f_list, NativeCode_x86 &ntv) {
	var_t *v;
	bool first_decl = false;

	if(var->get_type() == AST_VARIABLE) {
		if((v = ((VariableAST *)var)->get(f, f_list)) == NULL) {
			v = ((VariableAST *)var)->append(f, f_list);
			first_decl = true;
		}
	} else if(var->get_type() == AST_VARIABLE_DECL) {
		if((v = ((VariableDeclAST *)var)->get(f, f_list)) == NULL) {
			v = ((VariableDeclAST *)var)->append(f, f_list);
			first_decl = true;
		}
	} else if(var->get_type() == AST_VARIABLE_INDEX) {
		VariableIndexAST *via = (VariableIndexAST *)var;
			if(via->var->get_type() != AST_VARIABLE) error("error: variable");
		v = ((VariableAST *)via->var)->get(f, f_list);
			if(v == NULL) error("error: the var was not declared");
		codegen_expression(f, f_list, via->idx);
		ntv.genas("push eax");
		ntv.gencode(0x8b); ntv.gencode(0x4d);
			ntv.gencode(256 - ADDR_SIZE * v->id); // mov ecx [ebp-n]
		ntv.genas("push ecx");
			codegen_expression(f, f_list, src);
		ntv.genas("pop ecx");
		ntv.genas("pop edx");
		if(IS_TYPE(v->type, T_STRING)) {
			ntv.gencode(0x89); ntv.gencode(0x04); ntv.gencode(0x11); // mov [ecx+edx], eax
		} else { 
			ntv.gencode(0x89); ntv.gencode(0x04); ntv.gencode(0x91); // mov [ecx+edx*4], eax
		}
		return;
	}

	int ty = codegen_expression(f, f_list, src);
	if(v->is_global) {
		ntv.gencode(0xa3); f_list.append_addr_of_global_var(v->name, ntv.count); 
		ntv.gencode_int32(0x00000000); // GLOBAL_INSERT
	} else {
		if(v->type == T_STRING) {
			ntv.gencode(0x89); ntv.gencode(0x04); ntv.gencode(0x24); // mov [esp], eax
			ntv.gencode(0xff); ntv.gencode(0x56); ntv.gencode(96);// call *96(esi) str_copy
		}
		ntv.gencode(0x89); ntv.gencode(0x45);
			ntv.gencode(256 - ADDR_SIZE * v->id); // mov var eax
		if(first_decl && var->get_type() == AST_VARIABLE) v->type = ty; // for type inference
		if(v->type & T_ARRAY || v->type == T_STRING) { // append root for GC
			ntv.genas("push eax");
				ntv.gencode(0x8d); ntv.gencode(0x45);
					ntv.gencode(256 - ADDR_SIZE * v->id); // lea eax [esp - id*ADDR_SIZE]
				ntv.gencode(0x89); ntv.gencode(0x04); ntv.gencode(0x24); // mov [esp], eax
				ntv.gencode(0xff); ntv.gencode(0x56); ntv.gencode(44);// call *44(esi) LitMemory::append_ptr
			ntv.genas("pop eax");
		}
	}
}

var_t *VariableDeclAST::get(Function &f, Module &f_list) {
	if(info.is_global) return f_list.var_global.get(info.name, info.mod_name);
	return f.var.get(info.name, info.mod_name);
}

var_t *VariableDeclAST::append(Function &f, Module &f_list) {
	if(info.is_global) return f_list.var_global.append(info.name, info.type, true);
	return f.var.append(info.name, info.type);
}

int VariableIndexAST::codegen(Function &f, Module &f_list, NativeCode_x86 &ntv) {
	int ty = codegen_expression(f, f_list, var);
	ntv.genas("push eax");
	codegen_expression(f, f_list, idx);
	ntv.genas("mov ecx eax");
	ntv.genas("pop edx");
	if(IS_TYPE(ty, T_STRING)) {
		ntv.gencode(0x0f); ntv.gencode(0xb6); ntv.gencode(0x04); ntv.gencode(0x0a);// movzx eax, [edx + ecx]
		return T_CHAR;
	} else {
		ntv.gencode(0x8b); ntv.gencode(0x04); ntv.gencode(0x8a);// mov eax, [edx + ecx * 4]
		return ((T_STRING | T_ARRAY) == ty) ? T_STRING : T_INT;
	}
}

void VariableAST::codegen(Function &f, Module &f_list, NativeCode_x86 &ntv) {
	var_t *v;
	if(info.is_global) {
		v = f_list.var_global.get(info.name, info.mod_name);
		if(v == NULL) v = f_list.append_global_var(info.name, info.type); // global variable can be used if didn't declared
	} else 
		v = f.var.get(info.name, info.mod_name);
	if(v == NULL) error("error: '%s' was not declared", info.name.c_str());
	if(info.is_global == false) {
		ntv.gencode(0x8b); ntv.gencode(0x45);
			ntv.gencode(256 - ADDR_SIZE * v->id); // mov eax variable
	} else { // global
		ntv.gencode(0xa1); f_list.append_addr_of_global_var(v->name, ntv.count);
			ntv.gencode_int32(0x00000000); // mov eax GLOBAL_ADDR GLOBAL_INSERT
	}
}
var_t *VariableAST::get(Function &f, Module &f_list) {
	if(info.is_global) return f_list.var_global.get(info.name, info.mod_name);
	return f.var.get(info.name, info.mod_name);
}
var_t *VariableAST::append(Function &f, Module &f_list) {
	if(info.is_global) return f_list.append_global_var(info.name, info.type);
	return f.var.append(info.name, info.type);
}

void ReturnAST::codegen(Function &f, Module &f_list, NativeCode_x86 &ntv) {
	codegen_expression(f, f_list, expr);
	ntv.gencode(0xe9);
		f.return_list.push_back(ntv.count);
		ntv.gencode_int32(0x00000000);
}

void BreakAST::codegen(Function &f, Module &f_list, NativeCode_x86 &ntv) {
	ntv.gencode(0xe9); // jmp
		f.break_list.top()->push_back(ntv.count);
		ntv.gencode_int32(0x00000000);
}

int ArrayAST::codegen(Function &f, Module &f_list, NativeCode_x86 &ntv) {
	ntv.genas("mov eax %d", elems.size() * ADDR_SIZE);
	ntv.gencode(0x89); ntv.gencode(0x44); ntv.gencode(0x24); ntv.gencode(0 * ADDR_SIZE); // mov [esp+arg*ADDR_SIZE], eax
	ntv.genas("mov eax 4");
	ntv.gencode(0x89); ntv.gencode(0x44); ntv.gencode(0x24); ntv.gencode(1 * ADDR_SIZE); // mov [esp+arg*ADDR_SIZE], eax
	ntv.gencode(0xff); ntv.gencode(0x56); ntv.gencode(12); // call malloc
	ntv.genas("push eax");
	uint32_t a = 0;
	int ty;
	for(ast_vector::iterator it = elems.begin(); it != elems.end(); ++it) {
		ty = codegen_expression(f, f_list, *it);
		ntv.genas("pop ecx");
		ntv.genas("mov edx %d", a);
		ntv.gencode(0x89); ntv.gencode(0x81); ntv.gencode_int32(a); // mov [ecx+a], eax
		ntv.genas("push ecx");
		a += 4;
	}
	ntv.genas("pop eax");
	return ty | T_ARRAY;
}

void StringAST::codegen(Function &f, NativeCode_x86 &ntv) {
	ntv.gencode(0xb8);
		char *embed = (char *)LitMemory::alloc_const(str.length() + 1); // TODO: fix!
		replace_escape(strcpy(embed, str.c_str()));
	ntv.gencode_int32((uint32_t)embed); // mov eax string_address
}

void CharAST::codegen(Function &f, NativeCode_x86 &ntv) {
	ntv.genas("mov eax %d", ch);
}

void NumberAST::codegen(Function &f, NativeCode_x86 &ntv) {
	ntv.genas("mov eax %d", number);
}
