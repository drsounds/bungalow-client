//
// Created by Alexander Forselius on 2025-12-12.
//

#ifndef SPACIFYCLIENT_SPIDER_H
#define SPACIFYCLIENT_SPIDER_H
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "elua.h"
#include <vector>
#include "SpiderView.h"

namespace spider {
	class Spider {
		space::Space *space;
		vector<SpiderView *> views;
	public:
		explicit Spider::Spider(space::Space *);
		virtual string request(const string& method, const string& uri);
		void registerView(SpiderView *view);
	};
}

#endif //SPACIFYCLIENT_SPIDER_H