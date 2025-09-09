#pragma once
#include <array>
#include <map>
#include <string>

#include "core/ExportDLL.hpp"
#include <glm/glm.hpp>

#include "stb_truetype.h"

namespace TechEngine {
    struct CharInfo {
        glm::vec2 size;
        glm::vec2 bearing;
        float advance;
        glm::vec2 uv0;
        glm::vec2 uv1;
    };
    ;

    class CLIENT_DLL Font {
    public:
        uint32_t m_atlasTextureID = -1;
        int m_atlasWidth = 512;
        int m_atlasHeight = 512;

        std::array<stbtt_packedchar, 95> m_packedChars;
        float m_ascent = 0.0f;
        float m_nativeFontSize = 0.0f;

    public:
        Font();

        ~Font();

        bool load(const std::string& fontPath, int fontSize);

        uint32_t getAtlasTextureID() const;

        bool getCharInfo(char character, CharInfo& outInfo) const;

        float getAscent() const;

        float measureTextWidth(const std::string& text, float fontSize) const;
    };
}
