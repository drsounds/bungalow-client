#include "SPImage.h"
#include <stdio.h>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <string>
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
void abort_(char *text, int t, png_byte byte) {
    throw std::runtime_error(text);
}
void abort_(char *text, char *fileName) {
    throw std::runtime_error(std::string(text) + ": " + fileName);
}
void abort_(char *text) {
    throw std::runtime_error(text);
}
SPImage::SPImage(const string& fileName) {

    char *file_name = (char *)fileName.c_str();
    png_bytep header[8];    // 8 is the maximum size that can be checked

    /* open file and test for it being a png */
    FILE *fp = fopen(file_name, "rb");
    if (!fp)
            abort_("[read_png_file] File %s could not be opened for reading", file_name);
    fread(header, 1, 8, fp);
    if (png_sig_cmp((png_bytep)header, 0, 8))
            abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);


    /* initialize stuff */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr)
            abort_("[read_png_file] png_create_read_struct failed");

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
            abort_("[read_png_file] png_create_info_struct failed");

    if (setjmp(png_jmpbuf(png_ptr)))
            abort_("[read_png_file] Error during init_io");

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);

    png_read_info(png_ptr, info_ptr);

    width = png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);
    this->numPixels = width * height;
    this->pixels = new pixel[numPixels];
    color_type = png_get_color_type(png_ptr, info_ptr);
    bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    number_of_passes = png_set_interlace_handling(png_ptr);
    // Normalise every PNG flavour to 8-bit RGBA.
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png_ptr);
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);
    if (bit_depth == 16)
        png_set_strip_16(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);
    if (!(color_type & PNG_COLOR_MASK_ALPHA) && !png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
    png_read_update_info(png_ptr, info_ptr);


    /* read file */
    if (setjmp(png_jmpbuf(png_ptr)))
            abort_("[read_png_file] Error during read_image");

    row_pointers = (png_bytep *) new png_bytep[ height];
    for (y=0; y<height; y++)
            row_pointers[y] = (png_byte*) new png_byte [png_get_rowbytes(png_ptr,info_ptr)];
    if (png_ptr == NULL) {

        cout << "PNG pointer null";
    }
    if (row_pointers == NULL) {

        cout << "Row pointers null";
    }
    png_read_image(png_ptr, row_pointers);

    fclose(fp);

    // Process pixels

    if (png_get_color_type(png_ptr, info_ptr) != PNG_COLOR_TYPE_RGBA &&
        png_get_color_type(png_ptr, info_ptr) != PNG_COLOR_TYPE_RGB)
            abort_("[process_file] color_type of input file must be PNG_COLOR_TYPE_RGBA (%d) (is %d)",
                   PNG_COLOR_TYPE_RGBA, png_get_color_type(png_ptr, info_ptr));

    cout << "Pixels: " << numPixels << "\r\n";
    unsigned int i = 0;
    for (unsigned int y = 0; y < height; y++) {
            png_byte* row = row_pointers[y];
            for (unsigned x = 0; x < width; x++) {
                png_byte* ptr = &(row[x*4]);
                pixel pix;

                pix.r = ptr[0];
                pix.g = ptr[1];
                pix.b = ptr[2];
                pix.a = ptr[3];

                this->pixels[i] = pix;
                i++;
            }
    }

    for (y = 0; y < height; y++)
        delete[] row_pointers[y];
    delete[] row_pointers;
    row_pointers = nullptr;
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    this->handle = NULL;
}
SPImage::SPImage(unsigned int width, unsigned int height) {
    this->pixels = new pixel[width * height]();
    this->width = width;
    this->height = height;
    this->numPixels = width * height;
    this->handle = NULL;
}
SPImage *SPImage::sliceImage(unsigned int x, unsigned int y, unsigned int width, unsigned int height, int leftBorder, int topBorder, int bottomBorder, int rightBorder) {
    // Clip the region to this image.
    if (x > this->width) x = this->width;
    if (y > this->height) y = this->height;
    if (width > this->width - x) width = this->width - x;
    if (height > this->height - y) height = this->height - y;

    SPImage *image = new SPImage(width, height);
    for (unsigned int row = 0; row < height; row++) {
        const pixel *src = this->pixels + (y + row) * this->width + x;
        std::copy(src, src + width, image->pixels + row * width);
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
    delete[] pixels;
}
}
