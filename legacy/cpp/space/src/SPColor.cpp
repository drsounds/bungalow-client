#include "libspace.h"
#include "SPColor.h"
#include <stdio.h>
#include <string>
#include <algorithm>
#include <cmath>
using namespace std;
/*!
 * From http://www.dreamincode.net/forums/topic/58058-converting-rgb-to-hex/
 **/
string RGBToHex(int rNum, int gNum, int bNum) {
	string result;
    result.append("#");
	char r[255];
	snprintf(r, 255, "%.2X", rNum);
	result.append(r );

	char g[255];
	snprintf(g, 255, "%.2X", gNum);
	result.append(g );

	char b[255];
	snprintf(b, 255, "%.2X", bNum);
	result.append(b );

	return result;
}



namespace space {
/**
Colors
**/
short SPColor::getR() {
	return this->r;
}
short SPColor::getG(){
	return this->g;
}
short SPColor::getB(){
	return this->b;
}
short SPColor::getA() {
	return this->a;
}
string SPColor::toHTMLColor() {
    return RGBToHex(this->r, this->g, this->b);
}
void SPColor::setR(short r) {
	this->r = r;
}
void  SPColor::setG(short g) {
	this->g = g;
}
void  SPColor::setB(short b){
	this->b = b;
}

bool SPColor::empty() {
	bool ret = this->a == 0 && this->r == 0 && this->g == 0 && this->b == 0;
	return ret;
}

void  SPColor::setA(short a){
	this->a = a;
}

SPColor SPColor::alpha(short amount) const
{
	auto alpha = static_cast<short>(amount * 255);
	return {this->r, this->g, this->b, alpha};
}

SPColor SPColor::lighten(float luminance) const
{
	int red = static_cast<int>(r + (r * luminance));
	if (red > 255) red = 255;
	if (red < 0) red = 0;
	int green = static_cast<int>(g + (g * luminance));
	if (green > 255) green = 255;
	if (green < 0) green = 0;
	int blue = static_cast<int>(b + (b * luminance));
	if (blue > 255) blue = 255;
	if (blue < 0) blue = 0;

	return {static_cast<short>(red), static_cast<short>(green), static_cast<short>(blue), a};
}

SPColor::SPColor(short r, short g, short b, short a){
	this->r = r;
	this->g = g;
	this->b = b;
	this->a = a;
}

SPColor SPColor::tint(const SPColor& tint) const
{
	short red = this->r + (((255 - this->r) * (tint.r / 255.0)) * (float(tint.a) / 255.0));
	short green = this->g + (((255 - this->g) * (tint.g / 255.0)) * (float(tint.a) / 255.0));
	short blue = this->b + (((255 - this->b) * (tint.b / 255.0)) * (float(tint.a) / 255.0));
	short alpha = this->a + (((255 - this->a) * (tint.a / 255.0)) * (float(tint.a) / 255.0));
	return SPColor(red, green, blue, alpha);
}

SPColor SPColor::mix(const SPColor& other, float t) const
{
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;
	return SPColor(
		static_cast<short>(this->r + (other.r - this->r) * t),
		static_cast<short>(this->g + (other.g - this->g) * t),
		static_cast<short>(this->b + (other.b - this->b) * t),
		static_cast<short>(this->a + (other.a - this->a) * t)
	);
}

SPColor::SPColor() {
	this->r = 0;
	this->g = 0;
	this->b = 0;
	this->a = 0;
}

void SPColor::toHSL(float& h, float& s, float& l) const
{
    float rf = r / 255.0f, gf = g / 255.0f, bf = b / 255.0f;
    float mx = std::max({rf, gf, bf});
    float mn = std::min({rf, gf, bf});
    float delta = mx - mn;

    l = (mx + mn) / 2.0f;

    if (delta < 0.0001f) { h = 0.0f; s = 0.0f; return; }

    s = delta / (1.0f - std::abs(2.0f * l - 1.0f));

    if (mx == rf)       h = 60.0f * std::fmod((gf - bf) / delta, 6.0f);
    else if (mx == gf)  h = 60.0f * ((bf - rf) / delta + 2.0f);
    else                h = 60.0f * ((rf - gf) / delta + 4.0f);

    if (h < 0.0f) h += 360.0f;
}

SPColor SPColor::fromHSL(float h, float s, float l, short a)
{
    // Clamp inputs
    while (h < 0.0f)   h += 360.0f;
    while (h >= 360.0f) h -= 360.0f;
    if (s < 0.0f) s = 0.0f; if (s > 1.0f) s = 1.0f;
    if (l < 0.0f) l = 0.0f; if (l > 1.0f) l = 1.0f;

    float C = (1.0f - std::abs(2.0f * l - 1.0f)) * s;
    float hh = h / 60.0f;
    if (hh >= 6.0f) hh = 0.0f;
    float X = C * (1.0f - std::abs(std::fmod(hh, 2.0f) - 1.0f));
    float m = l - C / 2.0f;

    float r1, g1, b1;
    if      (hh < 1.0f) { r1 = C; g1 = X; b1 = 0; }
    else if (hh < 2.0f) { r1 = X; g1 = C; b1 = 0; }
    else if (hh < 3.0f) { r1 = 0; g1 = C; b1 = X; }
    else if (hh < 4.0f) { r1 = 0; g1 = X; b1 = C; }
    else if (hh < 5.0f) { r1 = X; g1 = 0; b1 = C; }
    else                { r1 = C; g1 = 0; b1 = X; }

    auto clamp = [](float v) -> short {
        int i = static_cast<int>((v + 0.0f) * 255.0f + 0.5f);
        if (i < 0) i = 0; if (i > 255) i = 255;
        return static_cast<short>(i);
    };
    return SPColor(clamp(r1 + m), clamp(g1 + m), clamp(b1 + m), a);
}

SPColor::SPColor(const string& html) {
	// TODO Fix this later
	int color = htmlToColor(html);

	short r, g, b, a = 0;
	if(html.length() > 6 + 1) {
		r = color >> (16 + 8) & 0xff;
		g = color >> 16 & 0xff;
		b = color >> 8 & 0xff;
	} else {
		r = color >> 16 & 0xff;
		g = color >> 8 & 0xff;
		b = color >> 0 & 0xff;
	}
	this->r = r;
	this->g = g;
	this->b = b;
	this->a = a;
}
};
