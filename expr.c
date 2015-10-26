#include "expr.h"

int32_t relExpr() {
	int andop=0, orop=0;
	condExpr();
	while((andop=skip("and") || skip("&")) || (orop=skip("or") || skip("|")) || skip("xor") || skip("^")) {
		genas("push eax");
		condExpr();
		genas("mov ebx eax");
		genas("pop eax");
		genCode(andop ? 0x21 : orop ? 0x09 : 0x31); genCode(0xd8); // and eax ebx
	}

	return 0;
}

int condExpr() {
	int32_t lt=0, gt=0, ne=0, eql=0, fle=0;
	addSubExpr();
	if((lt=skip("<")) || (gt=skip(">")) || (ne=skip("!=")) ||
			(eql=skip("==")) || (fle=skip("<=")) || skip(">=")) {
		genas("push eax");
		addSubExpr();
		genas("mov ebx eax");
		genas("pop eax");
		genCode(0x39); genCode(0xd8); // cmp %eax, %ebx
		/*
		 * < setl 0x9c
		 * > setg 0x9f
		 * <= setle 0x9e
		 * >= setge 0x9d
		 * == sete 0x94
		 * != setne 0x95
		 */
		genCode(0x0f); genCode(lt ? 0x9c : gt ? 0x9f : ne ? 0x95 : eql ? 0x94 : fle ? 0x9e : 0x9d); genCode(0xc0); // setX al
		genCode(0x0f); genCode(0xb6); genCode(0xc0); // movzx eax al
	}

	return 0;
}

int32_t addSubExpr() {
	int32_t add;
	mulDivExpr();
	while((add = skip("+")) || skip("-")) {
		genas("push eax");
		mulDivExpr();
		genas("mov ebx eax");  // mov %ebx %eax
		genas("pop eax");
		if(add) { genas("add eax ebx"); }// add %eax %ebx
		else { genas("sub eax ebx"); } // sub %eax %ebx
	}
	return 0;
}

int32_t mulDivExpr() {
  int32_t mul, div;
  primExpr();
  while((mul = skip("*")) || (div=skip("/")) || skip("%")) {
		genas("push eax");
    primExpr();
    genas("mov ebx eax"); // mov %ebx %eax
    genas("pop eax");
    if(mul) {
			genas("mul ebx");
    } else if(div) {
			genas("mov edx 0");
			genas("div ebx");
    } else { //mod
			genas("mov edx 0");
			genas("div ebx");
			genas("mov eax edx");
		}
  }
	return 0;
}

int32_t primExpr() {
  if(isdigit(tok.tok[tok.pos].val[0])) { // number?
    genas("mov eax %d", atoi(tok.tok[tok.pos++].val));
	} else if(skip("'")) { // char?
		genas("mov eax %d", tok.tok[tok.pos++].val[0]);
		skip("'");
	} else if(skip("\"")) { // string?
		genCode(0xb8); getString();
		genCodeInt32(0x00); // mov eax string_address
  } else if(isalpha(tok.tok[tok.pos].val[0])) { // variable or inc or dec
		char *name = tok.tok[tok.pos].val;
		Variable *v;

		if(isassign()) assignment();
		else {
			tok.pos++;
			if(skip("[")) { // Array?
		
				if((v = getVariable(name)) == NULL)
					error("error: %d: '%s' was not declared", tok.tok[tok.pos].nline, name);
				relExpr();
				genas("mov ecx eax");

				if(v->loctype == V_LOCAL) {
					genCode(0x8b); genCode(0x55); genCode(256 - sizeof(int32_t) * v->id); // mov edx, [ebp - v*4]
				} else if(v->loctype == V_GLOBAL) {
					genCode(0x8b); genCode(0x15); genCodeInt32(v->id); // mov edx, GLOBAL_ADDR
				}

				if(v->type == T_INT) {
					genCode(0x8b); genCode(0x04); genCode(0x8a);// mov eax, [edx + ecx * 4]
				} else {
					genCode(0x0f); genCode(0xb6); genCode(0x04); genCode(0x0a);// movzx eax, [edx + ecx]
				}
			
				if(!skip("]"))
					error("error: %d: expected expression ']'", tok.tok[tok.pos].nline);
			
			} else if(skip("(")) { // Function?
				if(!make_stdfunc(name)) {	// standard function
					func_t *function = getFunction(name, module);
					printf("addr: %d\n", function->address);
					if(isalpha(tok.tok[tok.pos].val[0]) || isdigit(tok.tok[tok.pos].val[0]) ||
						!strcmp(tok.tok[tok.pos].val, "\"") || !strcmp(tok.tok[tok.pos].val, "(")) { // has arg?
						for(int i = 0; i < function->args; i++) {
							relExpr();
							genas("push eax");
							skip(","); //TODO: add error handler 
						}
					}
					genCode(0xe8); genCodeInt32(0xFFFFFFFF - (ntvCount - function->address) - 3); // call func
					genas("add esp %d", function->args * sizeof(int32_t));
				}
				if(!skip(")")) error("func: error: %d: expected expression ')'", tok.tok[tok.pos].nline);
			} else {
				if((v = getVariable(name)) == NULL)
					error("var: error: %d: '%s' was not declared", tok.tok[tok.pos].nline, name);
				if(v->loctype == V_LOCAL) {
					genCode(0x8b); genCode(0x45);
					genCode(256 - sizeof(int32_t) * v->id); // mov eax variable
				} else if(v->loctype == V_GLOBAL) {
					genCode(0xa1); genCodeInt32(v->id); // mov eax GLOBAL_ADDR
				}
			}
		}
	} else if(skip("(")) {
    if(isassign()) assignment(); else relExpr();
		if(!skip(")"))
		 error("error: %d: expected expression ')'", tok.tok[tok.pos].nline);
  }

	while(isIndex()) make_index();

	return 0;
}

int32_t isIndex() {
	if(strcmp(tok.tok[tok.pos].val, "[") == 0) {
		return 1;
	}
	return 0;
}

int make_index() {
	genas("mov ecx eax");
	skip("["); relExpr(); skip("]");
	genCode(0x8b); genCode(0x04); genCode(0x81); // mov eax [eax * 4 + ecx]
	return 0;
}

