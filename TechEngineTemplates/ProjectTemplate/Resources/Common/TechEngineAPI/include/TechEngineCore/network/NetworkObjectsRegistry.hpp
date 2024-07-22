#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include "core/CoreExportDll.hpp"

namespace TechEngine {
    class NetworkObject;

    class CORE_DLL NetworkObjectsRegistry {
    private:
        static inline NetworkObjectsRegistry* instance;

        std::unordered_map<std::string, std::function<NetworkObject*()>> registry;

    public:
        NetworkObjectsRegistry();

        NetworkObjectsRegistry(const NetworkObjectsRegistry&) = delete;

        NetworkObjectsRegistry& operator=(const NetworkObjectsRegistry&) = delete;

        ~NetworkObjectsRegistry() = default;

        void init();

        static NetworkObjectsRegistry* getInstance();

        static void registerNetworkObject(const std::string& name, std::function<NetworkObject*()> factory);

        bool find(const std::string& name) const;

        std::function<NetworkObject*()> getFactory(const std::string& name) const;
    };
}
