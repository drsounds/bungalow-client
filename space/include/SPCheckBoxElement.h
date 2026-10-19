#ifndef SP_CHECKBOX_ELEMENT_H
#define SP_CHECKBOX_ELEMENT_H

#include "SPElement.h"

namespace space {
    class SPGraphicsContext;

    /// <checkbox checked="true">Label</checkbox>
    ///
    /// Toggles on click and then dispatches "click"; read isChecked() in the
    /// handler. The box is drawn by the skin (selector sp-checkbox, state
    /// "checked") with the label to its right.
    class SPCheckBoxElement : public SPElement {
    public:
        static const int kBoxSize = 16;

        explicit SPCheckBoxElement(SPElement* parent);

        string getType() override { return "checkbox"; }

        bool isChecked();
        void setChecked(bool checked);

        void pack() override;
        void paint(int x, int y, int width, int height, SPGraphicsContext* c) override;
        void onClick(int mouseButton, int x, int y) override;
    };
}

#endif
