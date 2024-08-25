#include "PathsBank.hpp"

#include <utility>

namespace TechEngine {
    PathsBank::PathsBank(std::filesystem::path rootPath) : m_rootPath(std::move(rootPath)) {
    }

    void PathsBank::init() {
        m_paths[translatePathType(PathType::Assets, AppType::Client)] = m_rootPath / "assets/client";
        m_paths[translatePathType(PathType::Assets, AppType::Common)] = m_rootPath / "assets/common";
        m_paths[translatePathType(PathType::Assets, AppType::Client)] = m_rootPath / "assets/client";

        m_paths[translatePathType(PathType::Resources, AppType::Client)] = m_rootPath / "resources/client";
        m_paths[translatePathType(PathType::Resources, AppType::Common)] = m_rootPath / "resources/common";
        m_paths[translatePathType(PathType::Resources, AppType::Server)] = m_rootPath / "resources/server";

        m_paths[translatePathType(PathType::Cache, AppType::Client)] = m_rootPath / "cache/client";
        m_paths[translatePathType(PathType::Cache, AppType::Common)] = m_rootPath / "cache/common";
        m_paths[translatePathType(PathType::Cache, AppType::Server)] = m_rootPath / "cache/server";
    }

    void PathsBank::shutdown() {
        System::shutdown();
        m_paths.clear();
    }

    const std::filesystem::path& PathsBank::getPath(PathType pathType, AppType appType) {
        return m_paths.at(translatePathType(pathType, appType));
    }

    int PathsBank::translatePathType(PathType pathType, AppType appType) {
        return static_cast<int>(pathType) * 10 + static_cast<int>(appType);
    }
}
