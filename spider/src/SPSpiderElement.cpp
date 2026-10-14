//
// Created by Alexander Forselius on 2025-12-12.
//

#include "SPSpiderElement.h"

#include "SPElement.h"

spider::SpiderElement::SpiderElement(SPElement *parent) {
  this->L = lua_open();
  elua_init_context(&this->elua_ctx);
  luaL_openlibs(this->L);
}
spider::SpiderElement::~SpiderElement() { 
}
void spider::SpiderElement::process(const string& method, const string& uri) { 
}

spider::SpiderElement::setTemplate(std::string filename) {
  this->m_template = filename;
  if((status = elua_loadfile(L, filename, &elua_ctx))) {
    fprintf(stderr, "Load error: %s\n", lua_tostring(L, -1));
	  lua_pop(L, 1); /* remove the error-msg and the function copy from the stack */
	}
  else { // Run
    if( (status = lua_pcall(L, 0, LUA_MULTRET, 0)) ) {
      // TODO: Print source only for the rows where the error occured, like python does.
      fprintf(stderr, "Runtime error: %s\n%s\n", lua_tostring(L, -1), elua_ctx.out->ptr);
			lua_pop(L, 1);
    }
  }
}