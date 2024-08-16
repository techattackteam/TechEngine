#pragma once
#include <Application.hpp>
#include <core/Client.hpp>
#include <core/Server.hpp>

namespace TechEngine {
    class Editor : public Application {
    private:
        Server server;
        Client client;

    public:
        void init() override;

        void update() override;

        void fixedUpdate() override;

        void destroy() override;
    };
}
