#include "SPCheckBoxElement.h"

#include <algorithm>

#include "SPColor.h"
#include "SPFontStyle.h"
#include "SPGraphicsContext.h"
#include "SPSkin.h"

namespace space {
    namespace {
        // Gap between the box and the label.
        const int kLabelGap = 6;
    }

    SPCheckBoxElement::SPCheckBoxElement(SPElement* parent) : SPElement(parent) {
        this->tagName = "checkbox";
    }

    bool SPCheckBoxElement::isChecked() {
        const string value = this->getStringAttribute("checked", "false");
        return value == "true" || value == "checked" || value == "1";
    }

    void SPCheckBoxElement::setChecked(bool checked) {
        this->setStringAttribute("checked", checked ? "true" : "false");
        this->invalidate();
    }

    void SPCheckBoxElement::pack() {
        SPElement::pack();
        if (this->getFlex() > 0) return;
        if (this->getHeight() <= 0)
            this->setHeight(kBoxSize + 4);
        if (this->getWidth() <= 0) {
            const string text = this->getInnerText();
            int textWidth = text.empty() ? 0 : kLabelGap + this->getTextBounds().width;
            this->setWidth(kBoxSize + textWidth);
        }
    }

    void SPCheckBoxElement::paint(int x, int y, int width, int height, SPGraphicsContext* c) {
        const bool checked = this->isChecked();
        const int boxSize = std::min(kBoxSize, height);
        const sp_rectangle box = { x, y + (height - boxSize) / 2, boxSize, boxSize, 0 };

        if (!this->getSkin()->drawElement(c, this, checked ? "checked" : "", box)) {
            // Skins without a checkbox sprite get a plain box and tick.
            const SPColor fg = this->getForegroundColor();
            c->fillRectangle(box.x, box.y, box.width, box.height, SPColor(255, 255, 255, 255));
            c->drawRectangle(box.x, box.y, box.width, box.height, fg);
            if (checked) {
                const int bx = box.x, by = box.y, s = box.width;
                c->drawLine(bx + s / 4, by + s / 2, bx + s * 2 / 5, by + s * 3 / 4, fg);
                c->drawLine(bx + s * 2 / 5, by + s * 3 / 4, bx + s * 3 / 4, by + s / 4, fg);
            }
        }

        const string text = this->getInnerText();
        if (text.empty()) return;
        const int fontSize = this->getIntegerAttribute("size", 11);
        const string fontFamily = this->getStringAttribute("font", "Tahoma");
        const SPFontStyle font(fontFamily, fontSize, fontSize / 2, false, false);
        const sp_rectangle textSize = c->measureString(text, font);
        const int textX = x + boxSize + kLabelGap;
        const SPColor color = this->getSkin()->getTextColor(this, checked ? "checked" : "")
            .value_or(this->getForegroundColor());
        c->drawString(text, font, color, textX, y + (height - textSize.height) / 2,
                      std::max(0, width - boxSize - kLabelGap), height);
    }

    void SPCheckBoxElement::onClick(int mouseButton, int x, int y) {
        this->setChecked(!this->isChecked());
        SPElement::onClick(mouseButton, x, y);
    }
}
