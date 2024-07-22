#pragma once
#include "core/CoreExportDll.hpp"
#include <string>
#include <unordered_map>
#include "network/NetworkObjectsRegistry.hpp"

namespace TechEngine {
    class CORE_DLL NetworkObject {
        friend class NetworkObjectsRegistry;
        friend class NetworkObjectsManager;

    private:
        unsigned int owner = -1;
        std::string networkUUID;

    public:
        std::unordered_map<std::string, int> variables;

        virtual ~NetworkObject();

        void registerVariable(const std::string& name, int value) {
            if (variables.find(name) == variables.end()) {
                variables[name] = value;
            }
        }

        virtual void onUpdate();

        virtual void onFixedUpdate();

        unsigned int getOwner() const;

        const std::string& getNetworkUUID() const;

    private:
        void setOwner(unsigned int owner);

        void setNetworkUUID(std::string networkUUID);
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
