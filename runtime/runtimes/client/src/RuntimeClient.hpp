#pragma once
#include "Application.hpp"
#include "core/Client.hpp"

namespace TechEngine {
    class RuntimeClient : public Application {
        Client m_client;

    public:
        void registerSystems() override;

        void init() override;

        void start() override;

        void fixedUpdate() override;

        void update() override;

        void stop() override;

        void shutdown() override;
    };
}
