#pragma once

#include <string>

namespace TechEngine {
    class ImGuiUtils {
    public:
        static bool beginMenuWithInputMenuField(const std::string &menuName, const std::string &hint, std::string &output);
    };
}
