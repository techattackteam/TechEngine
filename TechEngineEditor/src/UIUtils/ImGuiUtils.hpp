#pragma once

#include <string>
#include "glm/vec3.hpp"

namespace TechEngine::ImGuiUtils {
    bool beginMenuWithInputMenuField(const std::string &menuName, const std::string &hint, std::string &output);

    void drawVec3Control(const std::string &label, glm::vec3 &values, float resetValue = 0.0f, float columnWidth = 100.0f, float min = 0.0f, float max = 0.0f);

}

