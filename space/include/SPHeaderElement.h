#ifndef HEADERELEMENT_H
#define HEADERELEMENT_H
#include "SPHBoxElement.h"
namespace space {
    class SPImageElement;
    class SPElement;
    class SPTitleElement;
    class SPHeaderElement : public SPHBoxElement
    {
        public:
            SPHeaderElement();
            SPHeaderElement(SPElement *parent);
            virtual ~SPHeaderElement();
            SPTitleElement *title;
            SPImageElement *image;
            virtual string getType();
        protected:
        private:
    };
}

#endif // HEADERELEMENT_H
