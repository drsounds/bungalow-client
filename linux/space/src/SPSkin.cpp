#include "SPSkin.h"

#include "SPImage.h"
#include "SPColor.h"
#include "SPElement.h"

#include <iostream>

#include "SPAppHeaderElement.h"
#include "SPButtonElement.h"
#include "SPGraphicsContext.h"
#include "SPTabBarElement.h"
#include "SPTableElement.h"

namespace space {
SPSkin::SPSkin()
{
}

SPColor SPSkin::bgLight() const
{
    // Vista-style: very pale, slightly blue-tinted gray
    float l = 0.935f - m_lightness * 0.025f;
    return SPColor::fromHSL(m_hue, m_saturation * 0.11f, l);
}

SPColor SPSkin::bgDark() const
{
    // Same hue, dark navy — lightness is inverted relative to light
    float l = 0.10f + m_lightness * 0.06f;
    return SPColor::fromHSL(m_hue, m_saturation * 0.22f, l);
}

SPColor SPSkin::accent() const
{
    float l = 0.52f + m_lightness * 0.08f;
    return SPColor::fromHSL(m_hue, m_saturation * 0.85f, l);
}

SPColor SPSkin::tabStart() const
{
    // Lighter tropical start — aqua/cyan range
    return SPColor::fromHSL(m_hue, m_saturation * 0.90f, 0.74f + m_lightness * 0.06f);
}

SPColor SPSkin::tabEnd() const
{
    // Slightly deeper end with tropical hue shift, still light
    return SPColor::fromHSL(m_hue + 30.0f, m_saturation * 0.80f, 0.64f + m_lightness * 0.06f);
}

SPColor SPSkin::textColor(short scheme) const
{
    if (scheme == SP_COLOR_SCHEME_DARK)
        return SPColor::fromHSL(m_hue, 0.10f, 0.90f);
    // Dark blue in light mode — tied to the current hue for coherence
    return SPColor::fromHSL(m_hue, 0.55f, 0.17f);
}

slice::slice(SPImage *image, int x1, int y1, int x2, int y2, int leftBorder, int topBorder, int rightBorder, int bottomBorder) {
    this->image = image;
    this->x1 = x1;
    this->y1 = y1;
    this->x2 = x2;
    this->y2 = y2;
    this->leftBorder = leftBorder;
    this->topBorder = topBorder;
    this->rightBorder = rightBorder;
    this->bottomBorder = bottomBorder;
}

bool SPSkin::hasColor(string selector) {
    if(this->colors.find(selector) != this->colors.end()) {
        return true;
    }
    return false;
}

bool SPSkin::hasImage(string selector) {
    if(this->images.find(selector) != this->images.end()) {
        return true;
    }
    return false;
}
SPColor SPSkin::getColor(string selector, string def) {
    if (!this->hasColor(selector)) {
        return SPColor(255, 255, 255, 0);
    }
    return *space_pixel_to_colour(this->colors[selector]);
}
SPImage *SPSkin::getImage(string selector, string def) {
    if (!this->hasImage(selector)) {
        return NULL;
    }
    return this->images[selector];
}


SPSkin::~SPSkin()
{
    //dtor
}
}
