#pragma once
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace TechEngine {
    struct FileTypeHandler {
        std::vector<std::string> extensions;
        std::function<void(const std::filesystem::path&)> handler;
        std::function<void(const std::filesystem::path&)> onContextMenu;
        std::filesystem::path iconPath;
    };
}
