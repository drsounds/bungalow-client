#ifndef IMAGE_H
#define IMAGE_H
#include <string>
#include "SPColor.h"
#include <vector>
#include <functional>
#include <png.h>
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

            // PNG specific stuff

            png_byte color_type;
            png_byte bit_depth;

            png_structp png_ptr;
            png_infop info_ptr;
            int number_of_passes;
            png_bytep *row_pointers;
        protected:
        private:
    };
    #define IMAGE_CLASS
    #endif
}

#endif // IMAGE_H
