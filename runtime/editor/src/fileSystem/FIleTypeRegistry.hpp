#pragma once
#include <unordered_map>

#include "FileTypeHandler.hpp"

namespace TechEngine {
    class FIleTypeRegistry {
    private:
        std::unordered_map<std::string, FileTypeHandler> m_handlersByExtension;

    public:
        bool registerHandler(std::vector<std::string> extensions, FileTypeHandler handler);

        const FileTypeHandler& getHandler(const std::filesystem::path& path);
    };
}
