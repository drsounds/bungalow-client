#ifndef H_SPACE_WINDOW_ELEMENT
#define H_SPACE_WINDOW_ELEMENT
#include "SPElement.h"

#include <string>

using namespace std;

namespace space {
	class Space;
	class SPSkin;
	struct sp_event;
	class SPGraphicsContext;
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
		int notify(const string &event, SPObject *sender, SPEventArgs *data) override;
		void pack() override;
	};
};

#endif
