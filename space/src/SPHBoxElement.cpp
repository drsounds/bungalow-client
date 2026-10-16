#include "SPHBoxElement.h"

#include <algorithm>
#include <vector>

namespace space {
    SPHBoxElement::SPHBoxElement(SPElement *parent) : SPElement(parent) {
        this->setTagName("hbox");
    }

    void SPHBoxElement::pack() {
        SPElement::pack();
        std::vector<SPElement *> children;
        int totalFlex = 0;
        int fixedWidth = this->getPadding().left + this->getPadding().right;
        for (SPNode *node : this->getChildNodes()) {
            auto *child = static_cast<SPElement *>(node);
            if (!child->isVisible())
                continue;
            children.push_back(child);
            const sp_margin margins = child->getMargins();
            fixedWidth += margins.left + margins.right;
            if (child->getFlex() > 0)
                totalFlex += child->getFlex();
            else
                fixedWidth += child->getWidth();
        }

        const int gap = this->getGap();
        if (children.size() > 1)
            fixedWidth += gap * static_cast<int>(children.size() - 1);
        const int flexWidth = (std::max)(0, this->getWidth() - fixedWidth);
        int cumulativeFlex = 0;
        int assignedFlexWidth = 0;
        int x = this->getPadding().left;
        const int contentHeight = (std::max)(
            0, this->getHeight() - this->getPadding().top - this->getPadding().bottom);

        for (size_t i = 0; i < children.size(); ++i) {
            SPElement *child = children[i];
            const sp_margin margins = child->getMargins();
            child->setX(x + margins.left);
            child->setY(this->getPadding().top + margins.top);
            child->setHeight((std::max)(0, contentHeight - margins.top - margins.bottom));

            if (child->getFlex() > 0) {
                cumulativeFlex += child->getFlex();
                const int nextAssignedWidth = totalFlex > 0
                    ? static_cast<int>((static_cast<long long>(flexWidth) * cumulativeFlex) / totalFlex)
                    : assignedFlexWidth;
                child->setWidth(nextAssignedWidth - assignedFlexWidth);
                assignedFlexWidth = nextAssignedWidth;
            }

            x += margins.left + child->getWidth() + margins.right;
            if (i + 1 < children.size())
                x += gap;
        }

        for (SPElement *child : children)
            child->pack();
    }
}
