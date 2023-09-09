

#include <string>
#include <filesystem>

namespace TechEngine {
    namespace Paths {
        std::filesystem::path rootPath = std::filesystem::current_path();
        std::filesystem::path projectTemplate = rootPath.string() + "\\projectTemplate";
        std::filesystem::path scenesTemplate = projectTemplate.string() + "\\scenes\\DefaultScene.scene";
        std::filesystem::path scriptsTemplate = projectTemplate.string() + "\\scripts";
    }
}