#pragma once

#include "core/CoreExportDll.hpp"
#include <string>

namespace TechEngine {
    class CORE_DLL NetworkData {
    public:
        static void registerPacketType(const std::string& packetName);
    };
}
