#pragma once
#include "SPElement.h"
namespace space {
class SPGraphicsContext;
class SPTextElement : public SPElement
{
    public:
        SPTextElement(SPElement *parent);
        void pack() override;
        virtual ~SPTextElement();
        virtual string getType() override {
            return "text";
        }
};
}
