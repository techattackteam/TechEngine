#pragma once
#include "core/ExportDLL.hpp"
#include <glm/glm.hpp>

struct CLIENT_DLL UIVertex {
    glm::vec2 position;
    glm::vec4 color;
    glm::vec2 texCoords;
    //float textureIndex;

    UIVertex(const glm::vec2& pos, const glm::vec4& col, const glm::vec2& texCoords, float texIndex)
        : position(pos), color(col), texCoords(texCoords) {
    }
};
