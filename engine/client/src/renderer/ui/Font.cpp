#include "Font.hpp"
#define STB_TRUETYPE_IMPLEMENTATION
#include <filesystem>
#include <fstream>
#include <GL/glew.h>

#include "stb_truetype.h"
#include "core/Logger.hpp"
#include "renderer/ErrorCatcher.hpp"


namespace TechEngine {
    Font::Font() {
    }

    Font::~Font() {
        if (m_atlasTextureID != 0) {
            GlCall(glDeleteTextures(1, &m_atlasTextureID));
        }
    }

    bool Font::load(const std::string& fontPath, int fontSize) {
        m_nativeFontSize = fontSize;
        std::filesystem::path path(fontPath);
        if (!std::filesystem::exists(path) || std::filesystem::file_size(path) == 0) {
            TE_LOGGER_ERROR("Failed to open or empty font file: {0}", fontPath);
            return false;
        }
        auto size = std::filesystem::file_size(path);
        std::vector<unsigned char> ttfBuffer(size);
        std::ifstream file(path, std::ios::binary);
        if (!file.read(reinterpret_cast<char*>(ttfBuffer.data()), size)) {
            TE_LOGGER_ERROR("Failed to read font file: {0}", fontPath);
            return false;
        }
        file.close();

        stbtt_fontinfo fontInfo;
        if (!stbtt_InitFont(&fontInfo, ttfBuffer.data(), 0)) {
            TE_LOGGER_ERROR("Failed to init stbtt font");
            return false;
        }

        m_atlasWidth = 1028;
        m_atlasHeight = 1028;
        std::vector<unsigned char> atlasBitmap(m_atlasWidth * m_atlasHeight);
        stbtt_packedchar packedChars[95]; // ASCII 32–126

        stbtt_pack_context packContext;
        if (!stbtt_PackBegin(&packContext, atlasBitmap.data(), m_atlasWidth, m_atlasHeight, 0, 1, nullptr)) {
            TE_LOGGER_ERROR("Failed to initialize font packing context");
            return false;
        }
        const unsigned int h_oversample = 2;
        const unsigned int v_oversample = 2;
        stbtt_PackSetOversampling(&packContext, h_oversample, v_oversample);

        if (!stbtt_PackFontRange(&packContext, ttfBuffer.data(), 0, (float)fontSize, 32, 95, packedChars)) {
            TE_LOGGER_ERROR("Failed to pack font range");
            stbtt_PackEnd(&packContext);
            return false;
        }
        stbtt_PackEnd(&packContext);


        float scale = stbtt_ScaleForPixelHeight(&fontInfo, (float)fontSize);
        int ascent, descent, lineGap;
        stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

        m_ascent = (float)ascent * scale;
        m_lineHeight = (float)(ascent - descent + lineGap) * scale;

        GlCall(glGenTextures(1, &m_atlasTextureID));
        GlCall(glBindTexture(GL_TEXTURE_2D, m_atlasTextureID));
        GlCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, m_atlasWidth, m_atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, atlasBitmap.data()));
        GlCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        GlCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        GlCall(glBindTexture(GL_TEXTURE_2D, 0));

        std::copy(std::begin(packedChars), std::end(packedChars), m_packedChars.begin());

        return true;
    }

    float Font::measureTextWidth(const std::string& text, float fontSize) const {
        if (text.empty()) {
            return 0.0f;
        }

        if (m_nativeFontSize <= 0.0f) return 0.0f;
        const float scale = fontSize / m_nativeFontSize;

        float width = 0.0f;
        for (char c: text) {
            CharInfo info;
            if (getCharInfo(c, info)) {
                width += info.advance * scale;
            }
        }
        return width;
    }

    uint32_t Font::getAtlasTextureID() const {
        return m_atlasTextureID;
    }

    float Font::getLineHeight() const {
        return m_lineHeight;
    }

    bool Font::getCharInfo(char character, CharInfo& outInfo) const {
        if (character < 32 || character >= 127) {
            return false;
        }

        const stbtt_packedchar& pc = m_packedChars[character - 32];

        outInfo.advance = pc.xadvance;
        outInfo.bearing = {pc.xoff, pc.yoff};
        outInfo.size = {pc.xoff2 - pc.xoff, pc.yoff2 - pc.yoff};
        outInfo.uv0 = {(float)pc.x0 / m_atlasWidth, (float)pc.y0 / m_atlasHeight};
        outInfo.uv1 = {(float)pc.x1 / m_atlasWidth, (float)pc.y1 / m_atlasHeight};

        return true;
    }

    float Font::getAscent() const {
        return m_ascent;
    }
}
