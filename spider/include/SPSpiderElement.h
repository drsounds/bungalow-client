//
// Created by Alexander Forselius on 2025-12-12.
//

#ifndef BUNGALOWCLIENT_PYSPELEMENT_H
#define BUNGALOWCLIENT_PYSPELEMENT_H
#include <iostream>
#include <memory>
#include <lauxlib.h>
#include <string>
#include <map>
#include <thread>

#include "SPTabBarViewElement.h"


class Spider {

};

namespace space
{
	class SPElement;
	class SPSpiderElement : public SPTabBarViewElement {
		string m_template;
	public:
		explicit SPSpiderElement(SPElement *parent);
		virtual void process(string method, std::map<string, string> data);
		void setTemplate(const string&);
		string getTemplate() const {
			return m_template;
		}
		void render();
	};
}


#endif //BUNGALOWCLIENT_PYSPELEMENT_H