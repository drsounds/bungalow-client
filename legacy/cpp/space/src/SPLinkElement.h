#pragma once
#include "SPTextElement.h"

namespace space {
class SPElement;
class SPLinkElement : public SPTextElement
{
    public:
        SPLinkElement(SPElement *parent);
        virtual ~SPLinkElement();
        virtual string getType() override {
            return "link";
        }
    protected:
    private:
};
}

