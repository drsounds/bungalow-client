#ifndef INPUTELEMENT_H
#define INPUTELEMENT_H
#include "SPElement.h"
#include "SPGraphicsContext.h"
namespace space {
/// A text field drawn by the backend as a native control. Fires "enter"
/// (SPInputEventArgs with the text) when Return is pressed and "escape"
/// when Escape is.
class SPInputElement : public SPElement
{
private:
    private:
        string m_value;
        // Applied once the native control exists, i.e. on the next paint.
        bool m_valuePending = false;
        bool m_focusPending = false;
        // Whether the native control lies within every clipping ancestor.
        bool isInsideClip();
    public:
        SPInputElement();
        SPInputElement(SPElement *parent);
        virtual ~SPInputElement();
        virtual void paint(int x, int y, SPGraphicsContext *g) override;
        void setVisible(bool value) override;
        virtual string getType() override {
            return "input";
        }
        string getValue() const {
            return m_value;
        }
        void setValue(const string& value);
        /// Focuses the native control and selects its text once it is shown.
        void focus();
    protected:
    private:
};
}
#endif // INPUTELEMENT_H
