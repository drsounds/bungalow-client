#include "SPVBoxElement.h"

#include <algorithm>
#include <iostream>
#include <vector>

namespace space {
	SPVBoxElement::SPVBoxElement(SPElement *parent) : SPElement(parent) {
		this->setTagName("vbox");
	}

    void SPVBoxElement::pack() {
        SPElement::pack();
        std::vector<SPElement *> children;
        int totalFlex = 0;
        int fixedHeight = this->getPadding().top + this->getPadding().bottom;
        for (SPNode *node : this->getChildNodes()) {
            auto *child = static_cast<SPElement *>(node);
            if (!child->isVisible())
                continue;
            children.push_back(child);
            const sp_margin margins = child->getMargins();
            fixedHeight += margins.top + margins.bottom;
            if (child->getFlex() > 0)
                totalFlex += child->getFlex();
            else
                fixedHeight += child->getHeight();
        }

        const int gap = this->getGap();
        if (children.size() > 1)
            fixedHeight += gap * static_cast<int>(children.size() - 1);
        const int flexHeight = (std::max)(0, this->getHeight() - fixedHeight);
        int cumulativeFlex = 0;
        int assignedFlexHeight = 0;
        int y = this->getPadding().top;
        const int contentWidth = (std::max)(
            0, this->getWidth() - this->getPadding().left - this->getPadding().right);

        for (size_t i = 0; i < children.size(); ++i) {
            SPElement *child = children[i];
            const sp_margin margins = child->getMargins();
            child->setX(this->getPadding().left + margins.left);
            child->setY(y + margins.top);
            child->setWidth((std::max)(0, contentWidth - margins.left - margins.right));

            if (child->getFlex() > 0) {
                cumulativeFlex += child->getFlex();
                const int nextAssignedHeight = totalFlex > 0
                    ? static_cast<int>((static_cast<long long>(flexHeight) * cumulativeFlex) / totalFlex)
                    : assignedFlexHeight;
                child->setHeight(nextAssignedHeight - assignedFlexHeight);
                assignedFlexHeight = nextAssignedHeight;
            }

            y += margins.top + child->getHeight() + margins.bottom;
            if (i + 1 < children.size())
                y += gap;
        }

        for (SPElement *child : children)
            child->pack();
    };
}