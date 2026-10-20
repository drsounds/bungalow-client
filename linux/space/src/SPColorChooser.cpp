#include "SPColorChooser.h"
#include "SPGraphicsContext.h"
#include "SPEventArgs.h"
#include "SPMouseEventArgs.h"

namespace space {

static const std::vector<SPColor> kDefaultColors = {
    SPColor(220,  60,  60, 255),  // Red
    SPColor(230, 120,  40, 255),  // Orange
    SPColor(210, 190,  40, 255),  // Yellow
    SPColor( 60, 180,  60, 255),  // Green
    SPColor( 40, 180, 200, 255),  // Cyan
    SPColor( 80, 120, 240, 255),  // Blue
    SPColor(140,  70, 220, 255),  // Purple
    SPColor(220,  60, 140, 255),  // Pink
    SPColor(160, 110,  60, 255),  // Brown
    SPColor(100, 160, 140, 255),  // Teal
    SPColor(200, 200, 200, 255),  // Silver
    SPColor( 60,  60,  60, 255),  // Dark
};

SPColorChooser::SPColorChooser() {}

SPColorChooser::SPColorChooser(SPElement *parent) : SPElement(parent) {
    m_colors = kDefaultColors;
    m_selectedColor = kDefaultColors[5]; // Default: blue

    int rows = ((int)m_colors.size() + kCols - 1) / kCols;
    int w = kCols * (kSwatchSize + kSwatchGap) - kSwatchGap;
    int h = rows * (kSwatchSize + kSwatchGap) - kSwatchGap;
    this->setWidth(w);
    this->setHeight(h);
}

void SPColorChooser::paint(int x, int y, SPGraphicsContext *c) {
    this->absoluteBounds = {
        this->getX() + x, this->getY() + y, this->getWidth(), this->getHeight()
    };

    for (int i = 0; i < (int)m_colors.size(); i++) {
        int col = i % kCols;
        int row = i / kCols;
        int sx = absoluteBounds.x + col * (kSwatchSize + kSwatchGap);
        int sy = absoluteBounds.y + row * (kSwatchSize + kSwatchGap);

        c->fillRoundedRectangle(sx, sy, kSwatchSize, kSwatchSize, 4, m_colors[i]);

        // Draw selection indicator
        if (m_colors[i].toHTMLColor() == m_selectedColor.toHTMLColor()) {
            c->drawRoundedRectangle(sx - 2, sy - 2, kSwatchSize + 4, kSwatchSize + 4, 6,
                                    SPColor(255, 255, 255, 255));
            c->drawRoundedRectangle(sx - 1, sy - 1, kSwatchSize + 2, kSwatchSize + 2, 5,
                                    SPColor(30, 30, 30, 220));
        }
    }
}

int SPColorChooser::notify(const string& event, SPObject* sender, SPEventArgs* data) {
    if (event == "click") {
        auto *mouseArgs = dynamic_cast<SPMouseEventArgs *>(data);
        if (mouseArgs) {
            int relX = mouseArgs->getX() - this->absoluteBounds.x;
            int relY = mouseArgs->getY() - this->absoluteBounds.y;
            int idx = swatchIndexAt(relX, relY);
            if (idx >= 0 && idx < (int)m_colors.size()) {
                m_selectedColor = m_colors[idx];
                this->invalidate();
                if (onChange) onChange(m_selectedColor);
            }
        }
    }
    return SPElement::notify(event, sender, data);
}

int SPColorChooser::swatchIndexAt(int x, int y) const {
    if (x < 0 || y < 0) return -1;
    int col = x / (kSwatchSize + kSwatchGap);
    int row = y / (kSwatchSize + kSwatchGap);
    int localX = x % (kSwatchSize + kSwatchGap);
    int localY = y % (kSwatchSize + kSwatchGap);
    if (localX >= kSwatchSize || localY >= kSwatchSize) return -1;
    if (col >= kCols) return -1;
    int idx = row * kCols + col;
    if (idx >= (int)m_colors.size()) return -1;
    return idx;
}

void SPColorChooser::setSelectedColor(const SPColor& color) {
    m_selectedColor = color;
    this->invalidate();
}

} // namespace space
