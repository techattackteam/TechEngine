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

        m_atlasWidth = 512;
        m_atlasHeight = 512;
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

        GlCall(glGenTextures(1, &m_atlasTextureID));
        GlCall(glBindTexture(GL_TEXTURE_2D, m_atlasTextureID));
        GlCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, m_atlasWidth, m_atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, atlasBitmap.data()));
        GlCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        GlCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        GlCall(glBindTexture(GL_TEXTURE_2D, 0));

        std::copy(std::begin(packedChars), std::end(packedChars), m_packedChars.begin());

        return true;
    }

    uint32_t Font::getAtlasTextureID() const {
        return m_atlasTextureID;
    }

    bool Font::getQuad(char character, float& xpos, float& ypos, FontQuad& outQuad) const {
        // Ensure the character is in the packed range (ASCII 32-126)
        if (character < 32 || character >= 127) {
            return false;
        }

        stbtt_aligned_quad q;
        stbtt_GetPackedQuad(m_packedChars.data(), m_atlasWidth, m_atlasHeight, character - 32, &xpos, &ypos, &q, 0);

        // Transfer the data to our FontQuad struct
        outQuad.position0 = {q.x0, q.y0};
        outQuad.position1 = {q.x1, q.y1};
        outQuad.uv0 = {q.s0, q.t0};
        outQuad.uv1 = {q.s1, q.t1};

        return true;
    }

    float Font::getAscent() const {
        return m_ascent;
    }
}
