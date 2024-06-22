#pragma once
#include <string>
#include "core/CoreExportDll.hpp"

namespace TechEngine {
    class CORE_DLL FilePaths {
    public:
        std::string resourcesPath;
        std::string assetsPath;
        std::string commonResourcesPath;
        std::string commonAssetsPath;
        std::string cachPath;
    };
};
