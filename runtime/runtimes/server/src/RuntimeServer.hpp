#pragma once
#include "Application.hpp"
#include "core/Server.hpp"


namespace TechEngine {
    class RuntimeServer : public Application {
    private:
        Server m_server;

    public:
        void init() override;

        void start() override;

        void update() override;

        void fixedUpdate() override;

        void stop() override;

        void destroy() override;
    };
}
