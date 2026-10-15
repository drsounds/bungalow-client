#include "SPImage.h"
#include <stdio.h>
#include <iostream>
#include <cstdlib>
using namespace std;

namespace space {
SPColor *space_pixel_to_colour(pixel *pixel) {
    return new SPColor(pixel->r, pixel->g, pixel->b, pixel->a);
}
string space_pixel_to_hex(pixel *pixel) {
	string result;
    result.append("#");
	char r[255];
	snprintf(r, 255, "%.2X", pixel->r);
	result.append(r );

	char g[255];
	snprintf(g, 255, "%.2X", pixel->g);
	result.append(g );

	char b[255];
	snprintf(b, 255, "%.2X", pixel->b);
	result.append(b );

	return result;
}

SPImage::SPImage()
{
    //ctor
}
SPImage::SPImage(const string& fileName) {
    //
}
SPImage::SPImage(unsigned int width, unsigned int height) {
    this->pixels = new pixel[width * height];
    this->width = width;
    this->height = height;
    this->numPixels = width * height;
    this->handle = NULL;
}
SPImage *SPImage::sliceImage(unsigned int x, unsigned int y, unsigned int width, unsigned int height, int leftBorder, int topBorder, int bottomBorder, int rightBorder) {
    unsigned int startPos = x + (y * width);
    cout << "Start pos: " << startPos << "\r\n";
    SPImage *image = new SPImage(width, height);
    unsigned int length = width * height;


    for (unsigned int w = 0, h = 0, pos = 0, i = startPos; h < height; pos++, i++, w++) {
        pixel pix = {0};

        pix.r = this->pixels[i].r;
        pix.g = this->pixels[i].g;
        pix.b = this->pixels[i].b;
        pix.a = this->pixels[i].a;
        //cout << "Num pixels: " << image->numPixels << " pos: " << i << "\r\n";
        image->pixels[pos] = pix;
        if (w >= width) {
            h += 1;
            // cout << "Subliminal width: " << this->width - width << endl;
            i += this->width - width;
            w = 0;
            // cout << "W: " << w << "H:" << h << "\r\n";
        }
    }
    image->leftBorder = leftBorder;
    image->topBorder = topBorder;
    image->rightBorder = rightBorder;
    image->bottomBorder = bottomBorder;
    return image;
}
pixel SPImage::getPixel(unsigned int x, unsigned int y) {
    unsigned int pos = x + (this->width * y);
    pixel pix = this->pixels[pos];
    return pix;
}

pixel *SPImage::getPixelPtr(unsigned int x, unsigned int y) {
    unsigned int pos = x + (this->width * y);
    pixel *pix = &this->pixels[pos];
    return pix;
}

SPImage::~SPImage()
{
    if (handle && handleDeleter)
        handleDeleter(handle);
}
}
