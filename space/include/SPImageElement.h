#ifndef IMAGEELEMENT_H
#define IMAGEELEMENT_H

#include "SPElement.h"
#include "SPGraphicsContext.h"
#include "Space.h"



#include <string>

namespace space {
class SPGraphicsContext;
class SPImage;
class SPImageElement : public SPElement
{
    public:
        SPImageElement(SPElement *parent);
        virtual ~SPImageElement();
        void paint(int x, int y, SPGraphicsContext *g) override;
        virtual string getType() override {
            return "img";
        }
        string getSrc() {
            return this->m_uri;
        }
        void setSrc(const string& value) {
            this->m_uri = value;
            if (this->getSpace() != nullptr) {
                this->m_image = this->getSpace()->getGraphicsContext()->loadImage(value);
            }
        }
    protected:
    private:
        string m_uri;
        SPImage *m_image = nullptr;
};
}

#endif // IMAGEELEMENT_H
