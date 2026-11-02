#include "swimming/Render.h"

#include <cstdio>
#include <filesystem>
#include <memory>
#include <stdexcept>

#include <png.h>

namespace swimming {
    void savePng(const Pixmap& p, const std::string& path) {
#ifdef _WIN32
        std::unique_ptr<FILE, int (*)(FILE*)> file(_wfopen(std::filesystem::u8path(path).c_str(), L"wb"), &fclose);
#else
        std::unique_ptr<FILE, int (*)(FILE*)> file(std::fopen(path.c_str(), "wb"), &fclose);
#endif
        if (!file) throw std::runtime_error("cannot open " + path + " for writing");

        png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        png_infop info = png ? png_create_info_struct(png) : nullptr;
        if (!info) {
            png_destroy_write_struct(&png, nullptr);
            throw std::runtime_error("cannot create a PNG writer");
        }
        if (setjmp(png_jmpbuf(png))) {
            png_destroy_write_struct(&png, &info);
            throw std::runtime_error("cannot write " + path);
        }
        png_init_io(png, file.get());
        png_set_IHDR(png, info, p.width, p.height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
        png_write_info(png, info);
        for (unsigned y = 0; y < p.height; ++y)
            png_write_row(png, const_cast<png_bytep>(p.data.data() + static_cast<size_t>(y) * p.width * 4));
        png_write_end(png, nullptr);
        png_destroy_write_struct(&png, &info);
    }
}
