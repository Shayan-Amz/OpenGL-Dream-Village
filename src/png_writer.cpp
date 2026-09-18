// The vendored stb_image_write.h calls sprintf() in its HDR writer, which this
// project never uses.  MSVC's /sdl security check (and UCRT-based MinGW builds)
// report that call as error C4996, so the deprecation warning is disabled for
// this translation unit only, before any CRT header is pulled in.
#if defined(_MSC_VER) || defined(__MINGW32__)
#  ifndef _CRT_SECURE_NO_WARNINGS
#    define _CRT_SECURE_NO_WARNINGS
#  endif
#endif

#include "png_writer.h"

#include <cstring>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#include "stb_image_write.h"

namespace dv {

Image imageFromGlPixels(int width, int height, const unsigned char* bottomUpRgb)
{
    Image img;
    img.width = width;
    img.height = height;
    img.rgb.resize(static_cast<size_t>(width) * height * 3);
    const size_t rowBytes = static_cast<size_t>(width) * 3;
    for (int y = 0; y < height; ++y)
        std::memcpy(&img.rgb[static_cast<size_t>(y) * rowBytes],
                    bottomUpRgb + static_cast<size_t>(height - 1 - y) * rowBytes, rowBytes);
    return img;
}

bool writePng(const Image& img, const std::string& path)
{
    return stbi_write_png(path.c_str(), img.width, img.height, 3, img.rgb.data(), img.width * 3) != 0;
}

} // namespace dv
