#pragma once

#include <assets/ImageDecoder.hpp>

#include <vector>

namespace TechEngine {
    class MipBuilder {
    public:
        // Every level after the source down to 1x1, largest first. The source is not copied into the list, so the chain
        // has Texture::mipCount - 1 entries. A source with a side of 0 gives none.
        static void build(const FloatImage& source, std::vector<FloatImage>& out);

        // Each side is halved, and a side of 1 stays 1. An output pixel is the mean of the 2x2 pixels at twice its
        // position, and a tap past the edge repeats the last pixel, so an odd side loses its last row or column.
        static void downsample(const FloatImage& above, FloatImage& out);
    };
}
