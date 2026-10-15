#pragma once
#include "SPElement.h"
namespace space {
    class SPElement;
    class SPHBoxElement : public SPElement {

    public:
        explicit SPHBoxElement(SPElement *parent);
        virtual string getType() override {
            return "hbox";
        }
        SPHBoxElement() {}
        void pack() override;
    };
}
