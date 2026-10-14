//
// Created by Alexander Forselius on 2025-12-12.
//

#include "Spider.h"

#include <iostream>

using namespace std;
namespace spider {
	Spider::Spider(space::Space *space) {
		this->space = space;
	}

	Spider::~Spider() {
	}

	Spider::render() {

	}

	string Spider::request(const string& method, const string& uri) {
		string result = "";
		for (auto *view : this->views) {
			if (view->acceptsUri(uri)) {
				result = view->process(method, uri);
			}
		}
		return result;
	}

  void Spider::registerView(SpiderView *view) {
    this->views.push_back(view);
  }
	void Spider::serve() {

	}
}
