// Minimal PNG output used by the screenshot key and the head-less renderer.
#ifndef DREAM_VILLAGE_PNG_WRITER_H
#define DREAM_VILLAGE_PNG_WRITER_H

#include <string>
#include <vector>

namespace dv {

struct Image {
    int width = 0;
    int height = 0;
    std::vector<unsigned char> rgb;   // 3 bytes per pixel, top row first
};

// Builds an Image from a bottom-row-first RGB buffer as returned by glReadPixels.
Image imageFromGlPixels(int width, int height, const unsigned char* bottomUpRgb);

// Writes `img` as an 8-bit RGB PNG.  Returns false if the file could not be written.
bool writePng(const Image& img, const std::string& path);

} // namespace dv

#endif // DREAM_VILLAGE_PNG_WRITER_H
