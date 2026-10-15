#pragma once
#include <string>
#include "SPColor.h"
#include <vector>
#include <functional>

using namespace std;


namespace space {

    struct pixel {
        unsigned char r;
        unsigned char g;
        unsigned char b;
        unsigned char a;
    };

    /*!
     * Converts pixel to HTML color
     **/
    string space_pixel_to_hex(pixel *pixel);

    /*!
     * Converts pixel to Colour
     **/
    SPColor *space_pixel_to_colour(pixel *pixel);
    #ifndef IMAGE_CLASS
    class SPImage
    {

        public:
            int x, y;
            SPImage(unsigned int width, unsigned int height);
            SPImage();
            SPImage(const string& fileName);
            virtual ~SPImage();
            unsigned int width;
            unsigned int height;
            int leftBorder;
            int topBorder;
            int bottomBorder;
            int rightBorder;
            int numPixels;
            pixel *pixels;
            SPImage *sliceImage(unsigned int x,unsigned int y, unsigned int width, unsigned int height, int left, int top, int bottom, int right);
            pixel getPixel( unsigned int x, unsigned int y);
            pixel *getPixelPtr(unsigned int x, unsigned int y);
            void *handle; // OS Specific handle, created upon first load
            // Optional platform-supplied deleter for handle — called in ~SPImage
            std::function<void(void*)> handleDeleter;
        protected:
        private:
    };
    #define IMAGE_CLASS
    #endif
}

