#pragma once

#include "SPElement.h"
namespace space {
    class SPResource;
class SPResourceElement : public SPElement
{
    public:
        SPResourceElement(SPResource *resource, SPElement *parent);
        virtual ~SPResourceElement();
        void paint(int x, int y, SPGraphicsContext *g);
    protected:
        SPResource *resource;
    private:
};
}

