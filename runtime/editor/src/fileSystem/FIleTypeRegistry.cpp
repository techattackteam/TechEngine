#include "FIleTypeRegistry.hpp"

#include "core/Logger.hpp"

namespace TechEngine {
    bool FIleTypeRegistry::registerHandler(std::vector<std::string> extensions, FileTypeHandler handler) {
        for (const std::string& ext: extensions) {
            std::string extLower = std::string(ext);
            std::ranges::transform(extLower, extLower.begin(), ::tolower);
            if (m_handlersByExtension.contains(extLower)) {
                TE_LOGGER_WARN("Handler already registered for extension: {0}", extLower);
                return false;
            }
            m_handlersByExtension[extLower] = handler;
        }
        return true;
    }

    const FileTypeHandler& FIleTypeRegistry::getHandler(const std::filesystem::path& path) {
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
        if (m_handlersByExtension.contains(ext)) {
            return m_handlersByExtension[ext];
        }
        return m_handlersByExtension["*"];
    }
}
