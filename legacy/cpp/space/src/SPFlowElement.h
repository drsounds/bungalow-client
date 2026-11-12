#pragma once
#include "SPElement.h"
namespace space {
class SPFlowElement : public SPElement
{
    public:
        SPFlowElement();
        SPFlowElement(SPElement *parent);
        virtual ~SPFlowElement();
        void pack() override;
        virtual string getType() override {
            return "flow";
        }
    protected:
    private:
};
}
