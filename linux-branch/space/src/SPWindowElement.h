#pragma once
#include "SPElement.h"

#include <string>

using namespace std;

namespace space {
	class Space;
	class SPSkin;
	struct sp_event;
	class SPGraphicsContext;
	class MainElement;
	class SPWindowElement : public SPElement {
	public:
		void draw(SPGraphicsContext *gc, int width, int height);
		explicit SPWindowElement(Space *, SPSkin *);
		void invalidateRegion(sp_rectangle rect);

		string getType() override
		{
			return "window";
		}
		virtual void navigate(const string& uri);
		void pack() override;
	};
};

