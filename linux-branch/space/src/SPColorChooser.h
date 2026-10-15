#pragma once

#include "SPElement.h"
#include "SPColor.h"
#include <functional>
#include <vector>

namespace space {

class SPColorChooser : public SPElement {
public:
    SPColorChooser();
    explicit SPColorChooser(SPElement *parent);
    ~SPColorChooser() override = default;

    string getType() override { return "colorchooser"; }

    void paint(int x, int y, SPGraphicsContext *c) override;
    int notify(const string& event, SPObject* sender, SPEventArgs* data) override;

    SPColor getSelectedColor() const { return m_selectedColor; }
    void setSelectedColor(const SPColor& color);

    /// Called when the user picks a swatch. Set by the owner.
    std::function<void(const SPColor&)> onChange;

private:
    std::vector<SPColor> m_colors;
    SPColor m_selectedColor;

    static constexpr int kSwatchSize = 28;
    static constexpr int kSwatchGap  = 4;
    static constexpr int kCols       = 8;

    int swatchIndexAt(int relX, int relY) const;
};

} // namespace space

