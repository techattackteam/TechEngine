#pragma once
#include "System.hpp"

#include <iostream>
#include <memory>
#include <typeindex>
#include <unordered_map>

#include "core/Logger.hpp"

namespace TechEngine {
    class System;

    class CORE_DLL SystemsRegistry {
    private:
        std::unordered_map<std::type_index, std::shared_ptr<System>> m_systems;
        std::vector<std::shared_ptr<System>> m_systemsList;

    public:
        // Create and register a system
        template<typename T, typename... Args>
        T& registerSystem(Args&&... args) {
            static_assert(std::is_base_of<System, T>::value, "T must derive from System<T>");
            assert(!hasSystem<T>());
            m_systems[typeid(T)] = std::make_shared<T>(std::forward<Args>(args)...);
            m_systemsList.emplace_back(m_systems[typeid(T)]);
            return *static_cast<T*>(m_systems.at(typeid(T)).get());
        }

        // Retrieve a system
        template<typename T>
        T& getSystem() {
            if (!hasSystem<T>()) {
                TE_LOGGER_ERROR("System not found: {0}", typeid(T).name());
                throw std::runtime_error("System not found");
            }
            return *static_cast<T*>(m_systems.at(typeid(T)).get());
        }

        template<typename T>
        bool hasSystem() {
            return m_systems.find(typeid(T)) != m_systems.end();
        }

        void onStart();

        void onUpdate();

        void onFixedUpdate();

        void onStop();

        void onShutdown();
    };
}
