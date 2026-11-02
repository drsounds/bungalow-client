#ifndef SPDROPDOWNELEMENT_H
#define SPDROPDOWNELEMENT_H

#include "SPButtonElement.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace space {

    // A button that shows the selected option's label and a disclosure arrow.
    // Clicking it opens a menu of the options; picking one updates the
    // selection, calls onChange and fires a "change" event.
    // Skins draw it like any other button (its type is "button").
    class SPDropdownElement : public SPButtonElement {
    public:
        // (value, label) pairs
        using Options = std::vector<std::pair<std::string, std::string>>;

        // Shows a menu of labels and returns the chosen index, or -1 if dismissed.
        // Set by the platform layer; without it the dropdown can't be opened.
        static std::function<int(const std::vector<std::string>&)> s_showMenu;

        explicit SPDropdownElement(SPElement *parent);

        void setOptions(const Options& options);
        void addOption(const std::string& value, const std::string& label);
        const Options& getOptions() const { return m_options; }

        // Selects the option with this value; unknown values show the value itself.
        void setSelectedValue(const std::string& value);
        const std::string& getSelectedValue() const { return m_selectedValue; }

        void paint(int x, int y, SPGraphicsContext *c) override;
        int onClicked(SPObject* sender, SPEventArgs* e);

        std::function<void(const std::string&)> onChange;

    private:
        Options     m_options;
        std::string m_selectedValue;

        void updateText();
    };

}

#endif // SPDROPDOWNELEMENT_H
