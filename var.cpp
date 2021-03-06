#include "var.h"
#include "parse.h"
#include "token.h"
#include "asm.h"
#include "util.h"
#include "lit.h"
#include "func.h"

var_t *Variable::get(std::string name, std::string mod_name) {
	// local var
	for(std::vector<var_t>::iterator it = local.begin(); it != local.end(); ++it) {
		if(name == it->name) {
			return &(*it);
		}
	}

	return NULL;
}

var_t * Variable::append(std::string name, int type, bool is_global, std::string c_name) {
	uint32_t sz = local.size();
	var_t v = {
		.name = name,
		.type = type,
		.class_type = c_name,
		.id = sz + 1, 
		.is_global = is_global
	};
	local.push_back(v);
	return &local.back();
}

size_t Variable::total_size() {
	return local.size() * ADDR_SIZE;
}

