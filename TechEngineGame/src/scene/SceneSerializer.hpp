#pragma once

#include <string>

namespace TechEngine {
    class SceneSerializer {
    private:

    public:
        static void serialize(const std::string &filepath);

        static bool deserialize(const std::string &filepath);

    };
}
