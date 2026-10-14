#include "SPGraphicsContext.h"

#include "SPSkin.h"

namespace space {
    void SPGraphicsContext::setOrigo(const int& x, const int& y) {
    }

    SPGraphicsContext::SPGraphicsContext()
    {
        this->m_space = nullptr;
    }

    SPGraphicsContext::SPGraphicsContext(Space *space) : SPGraphicsContext()
    {
        this->m_space = space;
    }

    void SPGraphicsContext::drawImageSlices(const map<string, slice *>& slices, string selector, int left, int top, int width, int height)
    {
        if(slices.find(selector) != slices.end()) {
            slice *slice = slices.at(selector);
            this->drawImagePart(slice->image, left, top, width, height, slice->x1, slice->y1, slice->x2, slice->y2);
        }
    }
}
