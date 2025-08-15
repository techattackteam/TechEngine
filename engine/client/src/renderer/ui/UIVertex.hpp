#pragma once
#include "core/ExportDLL.hpp"
#include <glm/glm.hpp>

struct CLIENT_DLL UIVertex {
    glm::vec2 position;
    glm::vec4 color;
    glm::vec2 texCoords;

    UIVertex() : position(0, 0), color(0, 0, 0, 0), texCoords(0, 0) {
    }
};
