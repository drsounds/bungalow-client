//
// Created by Alexander Forselius on 2025-12-12.
//

#pragma once

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