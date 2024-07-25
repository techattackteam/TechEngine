#pragma once

#include "core/CoreExportDll.hpp"
#include <string>

namespace TechEngine {
    class CORE_DLL NetworkData {
        friend class Server;
        friend class NetworkEngine;

    private:
        inline static int networkID = -1; // ID of the network this manager is connected to. -1 if not connected to any network. 0 is reserved for Server. 1...Inf for Clients.
    public:
        static void registerPacketType(const std::string& packetName);

        static const int& getNetworkID();
    };
}
