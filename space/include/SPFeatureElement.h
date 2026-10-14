//
// Created by Alexander Forselius on 2025-12-12.
//

#ifndef SPACIFYCLIENT_SPFEATURE_H
#define SPACIFYCLIENT_SPFEATURE_H

#include "SPViewStackElement.h"

namespace space {
	class SPViewElement;
	class SPFeatureElement : public SPViewStackElement
	{
		std::vector<SPViewElement*> m_views;
	public:
		void registerView(SPViewElement *view);
		explicit SPFeatureElement(SPElement *parent);
	};
}


#endif //SPACIFYCLIENT_SPFEATURE_H