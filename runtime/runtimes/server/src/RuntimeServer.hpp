#pragma once
#include "Application.hpp"
#include "core/Server.hpp"


namespace TechEngine {
    class RuntimeServer : public Application {
    private:
        Server server;

    public:
        void init() override;

        void update() override;

        void fixedUpdate() override;

        void destroy() override;
    };
}
