#pragma once
#include "SPElement.h"
#include "SPGraphicsContext.h"
namespace space {
class SPInputElement : public SPElement
{
private:
    private:
        string m_value;
    public:
        SPInputElement();
        SPInputElement(SPElement *parent);
        virtual ~SPInputElement();
        virtual void paint(int x, int y, SPGraphicsContext *g) override;
        virtual string getType() override {
            return "input";
        }
        string getValue() const {
            return m_value;
        }
        void setValue(const string& value);
    protected:
    private:
};
}
