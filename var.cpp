#include "var.h"
#include "parse.h"
#include "token.h"
#include "asm.h"
#include "util.h"
#include "lit.h"
#include "func.h"

var_t *Variable::get(std::string name, std::string mod_name) {
	// local var
	for(int i = 0; i < local.size(); i++) {
		if(name == local[i].name) {
			return &(local[i]);
		}
	}
	// global var
	for(std::vector<var_t>::iterator it = global.begin(); it != global.end(); it++) {
		if(name == it->name && mod_name == it->mod_name) {
			return &(*it);
		}
	}

	return NULL;
}

var_t * Variable::append(std::string name, int type, std::string c_name) {
	uint32_t sz = local.size();
	var_t v = {
		.name = name,
		.type = type,
		.class_type = c_name,
		.id = sz + 2, 
		.loctype = V_LOCAL
	};
	local.push_back(v);
	return &local.back();
}

size_t Variable::total_size() {
	return local.size() * ADDR_SIZE;
}

int Parser::is_asgmt() {
	if(tok.is("=", 1)) return 1;
	else if(tok.is("++", 1)) return 1;
	else if(tok.is("--", 1)) return 1;
	else if(tok.is("[", 1)) {
		int i = tok.pos + 2, t = 1;
re:
		while(t) {
			if(tok.at(i).val == "[") t++;
			if(tok.at(i).val == "]") t--;
			if(tok.at(i).val == ";")
				error("index: error: %d: invalid expression", tok.tok[tok.pos].nline);
			i++;
		}
		t = 1;
		if(tok.at(i).val == "[") { i++; goto re; }
#ifdef DEBUG
		std::cout << "> " << tok.tok[i].val << std::endl;
#endif
		if(tok.at(i).val == "=") return 1;
	} else if(tok.is(".", 1) /* module */ || 
			tok.is(":", 1) /* var:type */) {
		if(tok.is("=", 3) || tok.is("=", 5)) return 1;
	}
	return 0;
}

int Parser::asgmt() {
	std::string name = tok.tok[tok.pos].val, mod_name = "";
	if(tok.is(".", 1)) { // module's func or var?
		mod_name = tok.get().val;
		tok.pos += 2;
		name = tok.get().val;
	}

	int declare = 0;
	var_t *v = var.get(name, mod_name);

	if(v == NULL) v = var.get(name, module);
	if(v == NULL) { declare = 1; v = declare_var(); }
	SKIP_TOK;

	if(v->loctype == V_LOCAL) {
		if(tok.is("[")) { // Array?
			asgmt_array(v);
		} else { // Scalar?
			asgmt_single(v);
		}
	} else if(v->loctype == V_GLOBAL) {
		if(declare) { // declare for global var?
			// asgmt only int32_terger
			if(tok.skip("=")) {
				unsigned *m = (unsigned *)v->id; // v->id is gloval var's address
				*m = atoi(tok.tok[tok.pos++].val.c_str());
			}
		} else {
			if(tok.is("[")) { // Array?
				asgmt_array(v);
			} else asgmt_single(v);
		}
	}
	return 0;
}

int Parser::asgmt_single(var_t *v) {
	int inc = 0, dec = 0;

	if(v->loctype == V_LOCAL) { // local single
		if(tok.skip("=")) {
			expr_entry();
		} else if((inc=tok.skip("++")) || (dec=tok.skip("--"))) {
			ntv.gencode(0x8b); ntv.gencode(0x45);
			ntv.gencode(256 -
					(IS_TYPE(v->type, T_INT) ? ADDR_SIZE :
					 IS_TYPE(v->type, T_STRING) ? ADDR_SIZE :
					 IS_TYPE(v->type, T_DOUBLE) ? sizeof(double) : 4) * v->id); // mov eax varaible
			ntv.genas("push eax");
			if(inc) ntv.gencode(0x40); // inc eax
			else if(dec) ntv.gencode(0x48); // dec eax
		}
		ntv.gencode(0x89); ntv.gencode(0x45);
		ntv.gencode(256 -
				(IS_TYPE(v->type, T_INT) ? ADDR_SIZE :
				 IS_TYPE(v->type, T_STRING) ? ADDR_SIZE :
				 IS_TYPE(v->type, T_DOUBLE) ? sizeof(double) : 4) * v->id); // mov var eax
		if(inc || dec) ntv.genas("pop eax");
	} else if(v->loctype == V_GLOBAL) { // global single
		if(tok.skip("=")) {
			expr_entry();
			ntv.gencode(0xa3); ntv.gencode_int32(v->id); // mov GLOBAL_ADDR eax
		} else if((inc=tok.skip("++")) || (dec=tok.skip("--"))) {
			ntv.gencode(0xa1); ntv.gencode_int32(v->id);// mov eax GLOBAL_ADDR
			ntv.genas("push eax");
			if(inc) ntv.gencode(0x40); // inc eax
			else if(dec) ntv.gencode(0x48); // dec eax
			ntv.gencode(0xa3); ntv.gencode_int32(v->id); // mov GLOBAL_ADDR eax
			ntv.genas("pop eax");
		}
	}
	return 0;
}

int Parser::asgmt_array(var_t *v) {
	int inc = 0, dec = 0;

	if(!tok.skip("[")) error("error: %d: expected '['", tok.tok[tok.pos].nline);
	if(v->loctype == V_LOCAL) {
		ExprType et;// = expr_entry(); // TODO: fix
		ntv.genas("push eax");
		if(!tok.skip("]")) error("error: %d: ']' except", tok.tok[tok.pos].nline);
		// while(is_index()) make_index(et);

		if(tok.skip("=")) {
			expr_entry();
			ntv.gencode(0x8b); ntv.gencode(0x4d);
			ntv.gencode(256 -
					(IS_TYPE(v->type, T_INT) ? ADDR_SIZE :
					 IS_TYPE(v->type, T_STRING) ? ADDR_SIZE :
					 IS_TYPE(v->type, T_DOUBLE) ? sizeof(double) : 4) * v->id); // mov ecx [ebp-n]
			ntv.genas("pop edx");
			if(IS_ARRAY(v->type)) {
				ntv.gencode(0x89); ntv.gencode(0x04); ntv.gencode(0x91); // mov [ecx+edx*4], eax
			} else if(IS_TYPE(v->type, T_STRING)) {
				ntv.gencode(0x89); ntv.gencode(0x04); ntv.gencode(0x11); // mov [ecx+edx], eax
			}
		} else if((inc=tok.skip("++")) || (dec=tok.skip("--"))) {

		} else 
			error("error: %d: invalid asgmt", tok.tok[tok.pos].nline);
	} else if(v->loctype == V_GLOBAL) {
		expr_entry();
		ntv.genas("push eax");
		tok.skip("]");
		if(tok.skip("=")) {

			expr_entry();
			ntv.gencode(0x8b); ntv.gencode(0x0d); ntv.gencode_int32(v->id); // mov ecx GLOBAL_ADDR
			ntv.genas("pop edx");
			if(IS_ARRAY(v->type)) {
				ntv.gencode(0x89); ntv.gencode(0x04); ntv.gencode(0x91); // mov [ecx+edx*4], eax
			} else if(IS_TYPE(v->type, T_STRING)) {
				ntv.gencode(0x89); ntv.gencode(0x04); ntv.gencode(0x11); // mov [ecx+edx], eax
			}
		
		} else if((inc=tok.skip("++")) || (dec=tok.skip("--"))) {

		} else
			error("error: %d: invalid asgmt", tok.tok[tok.pos].nline);
	}

	return 0;
}

var_t *Parser::declare_var() {
	int npos = tok.pos;

	if(isalpha(tok.get().val[0])) {
		tok.pos++;
		int is_ary = 0;
		if(tok.skip(":")) {
			if(tok.is("int")) { 
				if(tok.is("[", 1) && tok.is("]", 2)) { tok.pos += 2; is_ary = T_ARRAY; }
				return var.append(tok.tok[npos].val, is_ary | T_INT); 
			} else if(tok.is("string")) { 
				if(tok.is("[", 1) && tok.is("]", 2)) { tok.pos += 2; is_ary = T_ARRAY; }
				return var.append(tok.tok[npos].val, is_ary | T_STRING); 
			} else if(tok.is("double")) { 
				if(tok.is("[", 1) && tok.is("]", 2)) { tok.pos += 2; is_ary = T_ARRAY; }
				return var.append(tok.tok[npos].val, is_ary | T_DOUBLE); // TODO: support array
			} else {
				std::string class_name = tok.get().val;
				if(tok.is("[", 1) && tok.is("]", 2)) { tok.pos += 2; is_ary = T_ARRAY; }
				return var.append(tok.tok[npos].val, is_ary | T_USER_TYPE, class_name);
			}
		} else { 
			tok.pos--;
			return var.append(tok.tok[npos].val, T_INT); 
		}
	} else error("error: %d: can't declare var", tok.tok[tok.pos].nline);
	return NULL;
}


