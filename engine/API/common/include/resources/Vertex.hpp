#pragma once

#include "common/include/core/ExportDLL.hpp"
#include "glm/glm.hpp"

namespace TechEngineAPI {
    struct API_DLL Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoords;
        glm::vec4 color;
    };
}
