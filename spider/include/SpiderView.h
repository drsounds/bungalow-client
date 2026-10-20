//
// Created by Alexander Forselius on 2025-12-12.
//

#ifndef SPIDER_VIEW_H
#define SPIDER_VIEW_H
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "elua.h"

namespace space {
class SPSpiderElement;
}

namespace spider {
	class SpiderView {
		lua_State *L;
	  elua_context_t elua_ctx;
	public:
		virtual void process(string method, const string& path, std::map<string, string> data);
		virtual bool acceptsUri(const string& uri);
		explicit Spider(space::SPSpiderElement *element);
		void serve();
	};
}

#endif //BUNGALOWCLIENT_SPIDER_H