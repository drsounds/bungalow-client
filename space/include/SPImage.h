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
            SPImage(const SPImage&) = delete;
            SPImage& operator=(const SPImage&) = delete;
            virtual ~SPImage();
            unsigned int width = 0;
            unsigned int height = 0;
            int leftBorder = 0;
            int topBorder = 0;
            int bottomBorder = 0;
            int rightBorder = 0;
            int numPixels = 0;
            pixel *pixels = nullptr; // owned, allocated with new[]
            /// Copies the width x height region at (x, y) into a new image.
            SPImage *sliceImage(unsigned int x,unsigned int y, unsigned int width, unsigned int height, int left, int top, int bottom, int right);
            pixel getPixel( unsigned int x, unsigned int y);
            pixel *getPixelPtr(unsigned int x, unsigned int y);
            void *handle = nullptr; // OS Specific handle, created upon first load
            // Optional platform-supplied deleter for handle — called in ~SPImage
            std::function<void(void*)> handleDeleter;

            // PNG specific stuff

            png_byte color_type = 0;
            png_byte bit_depth = 0;

            png_structp png_ptr = nullptr;
            png_infop info_ptr = nullptr;
            int number_of_passes = 0;
            png_bytep *row_pointers = nullptr;
        protected:
        private:
    };
    #define IMAGE_CLASS
    #endif
}

#endif // IMAGE_H
