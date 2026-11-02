#include "SPDropdownElement.h"
#include "SPGraphicsContext.h"
#include "SPColor.h"
#include "SPMouseEventArgs.h"

namespace space {

    std::function<int(const std::vector<std::string>&)> SPDropdownElement::s_showMenu;

    // Width reserved at the right edge for the arrow
    static const int kArrowArea = 20;

    SPDropdownElement::SPDropdownElement(SPElement *parent) : SPButtonElement(parent) {
        // Clicks arrive as "click" events dispatched through the tree, not via onClick()
        this->addEventListener("click", reinterpret_cast<SPCallback>(&SPDropdownElement::onClicked), this, this);
    }

    void SPDropdownElement::setOptions(const Options& options) {
        m_options = options;
        updateText();
    }

    void SPDropdownElement::addOption(const std::string& value, const std::string& label) {
        m_options.emplace_back(value, label);
        updateText();
    }

    void SPDropdownElement::setSelectedValue(const std::string& value) {
        m_selectedValue = value;
        updateText();
    }

    void SPDropdownElement::updateText() {
        std::string label = m_selectedValue;
        for (const auto& [value, text] : m_options) {
            if (value == m_selectedValue) {
                label = text;
                break;
            }
        }
        // Skins centre the label; trailing spaces keep it clear of the arrow.
        this->setInnerText(label + "    ");
    }

    void SPDropdownElement::paint(int x, int y, SPGraphicsContext *c) {
        SPButtonElement::paint(x, y, c);

        // Downward triangle, drawn as shrinking horizontal lines
        const sp_rectangle& b = this->absoluteBounds;
        const SPColor color = this->getForegroundColor();
        const int cx = b.x + b.width - kArrowArea / 2 - 2;
        const int top = b.y + b.height / 2 - 2;
        for (int i = 0; i < 4; ++i)
            c->drawLine(cx - 3 + i, top + i, cx + 4 - i, top + i, color);
    }

    int SPDropdownElement::onClicked(SPObject* /*sender*/, SPEventArgs* e) {
        // Right-clicks are dispatched as "click" too; only the left button opens the menu
        auto* me = dynamic_cast<SPMouseEventArgs*>(e);
        if (me && me->getButton() != 0) return 0;
        if (!s_showMenu || m_options.empty()) return 0;

        std::vector<std::string> labels;
        labels.reserve(m_options.size());
        for (const auto& option : m_options)
            labels.push_back(option.second);

        const int index = s_showMenu(labels);
        if (index < 0 || index >= static_cast<int>(m_options.size())) return 0;

        const std::string value = m_options[index].first;
        if (value == m_selectedValue) return 0;
        setSelectedValue(value);
        if (onChange) onChange(value);
        this->notify("change", this, nullptr);
        this->invalidate();
        return 0;
    }

}
