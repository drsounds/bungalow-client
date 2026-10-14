#ifndef APPHEADERELEMENT_H
#define APPHEADERELEMENT_H
#include "SPHBoxElement.h"

namespace space {
    class SPElement;
    class SPAppHeaderElement : public SPHBoxElement
    {
        public:
            explicit SPAppHeaderElement(SPElement *parent);
            SPAppHeaderElement() = default;
            ~SPAppHeaderElement() override;
            string getType() override
            {
                return "appheader";
            }
        protected:
        private:
    };
}

#endif // APPHEADERELEMENT_H
