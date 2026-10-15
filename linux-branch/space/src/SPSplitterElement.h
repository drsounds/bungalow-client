#pragma once
#include "SPElement.h"
namespace space {
class SPSplitterElement : public SPElement
{
    public:
		SPSplitterElement(SPElement *parent);
		void paint(int x, int y, int width, int height, SPGraphicsContext* c) override;
        virtual ~SPSplitterElement();
    protected:
    private:
};
}

