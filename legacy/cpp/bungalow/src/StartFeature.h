//
// Created by Alexander Forselius on 2025-12-12.
//

#pragma once

#include "../../space/src/SPFeatureElement.h"

namespace space {
	class StartFeature : public SPFeatureElement
	{
	public:
		explicit StartFeature(SPElement *parent);
	};
}