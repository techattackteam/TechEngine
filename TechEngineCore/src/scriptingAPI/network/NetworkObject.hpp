#pragma once
#include "core/CoreExportDll.hpp"
#include <string>
#include <unordered_map>

#include "NetworkVariable.hpp"
#include "events/network/SyncNetworkInt.hpp"
#include "network/NetworkObjectsRegistry.hpp"

namespace TechEngine {
    class NetworkVariable;

    class CORE_DLL NetworkObject {
        friend class NetworkObjectsRegistry;
        friend class NetworkObjectsManager;

    private:
        unsigned int owner = -1;
        std::string networkUUID;

    public:
        std::unordered_map<std::string, NetworkVariable*> variables; // name, value

        NetworkObject();

        virtual ~NetworkObject();

        void registerVariable(const std::string& name, NetworkVariable* variable);

        void unregisterVariable(const std::string& name);

        void requestVariablesCreation();

        virtual void onUpdate();

        virtual void onFixedUpdate();

        unsigned int getOwner() const;

        const std::string& getNetworkUUID() const;

        void requestChangeValue(const std::string& name, int value);

        bool isOwner();

        void updateNetworkVariables();

    private:
        void setOwner(unsigned int owner);

        void setNetworkUUID(std::string networkUUID);

        void onSyncNetworkVariable(SyncNetworkInt* event);

        bool isVariableRegistered(const std::string& name);
    };
}

#define RegisterNetworkObject(type) \
namespace {\
\
int registerNetworkObject() {\
    TechEngine::NetworkObjectsRegistry::registerNetworkObject(typeid(type).name(), []() {\
        return new type();\
    });\
    return 1;\
}\
\
int i = registerNetworkObject();\
}
