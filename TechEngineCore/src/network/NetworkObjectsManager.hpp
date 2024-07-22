#pragma once

#include "system/System.hpp"
#include "core/CoreExportDll.hpp"
#include <vector>
#include <string>

/**
 * Manages local instances of network objects.
 */
namespace TechEngine {
    class NetworkObject;

    class CORE_DLL NetworkObjectsManager : public System {
    private:
        std::vector<NetworkObject*> networkObjects{};

    public:
        NetworkObject* createNetworkObject(const std::string& name, unsigned int owner, std::string networkUUID);

        bool deleteNetworkObject(const std::string& networkUUID);

        const std::vector<NetworkObject*>& getNetworkObjects() const;

        void update();

        void fixedUpdate();

        void onDisconnect();
    };
}
