//
// Created by Alexander Forselius on 2025-12-12.
//

#include "SpiderView.h"

#include <iostream>

using namespace std;
namespace spider {
	SpiderView::SpiderView(space::Spider *) {
		this->L = lua_open();
		elua_init_context(&this->elua_ctx);
		luaL_openlibs(this->L);
	}

	SpiderView::~Spider() {
		elua_free_context(&elua_ctx);
		lua_close(L);
	}

	string SpiderView::process(const string& request, const string& uri) {
		status = lua_pcall(L, 0, LUA_MULTRET, 0);
	}
}
