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
        //int networkID = -1; // ID of the network this manager is connected to. -1 if not connected to any network. 0 is reserved for Server. 1...Inf for Clients.

    public:
        NetworkObject* createNetworkObject(const std::string& name, int owner, std::string networkUUID);

        bool deleteNetworkObject(const std::string& networkUUID);

        bool registerNetworkVariable(int owner, const std::string& uuid, const std::string& variableName, int value);

        const std::vector<NetworkObject*>& getNetworkObjects() const;

        void update();

        void fixedUpdate();

        void onDisconnect();
    };
}
