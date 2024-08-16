#pragma once
#include <Application.hpp>
#include <core/Client.hpp>
#include <core/Server.hpp>

namespace TechEngine {
    class Editor : public Application {
    private:
        Entry m_entry;
        Server m_server;
        Client m_client;
        SystemsRegistry m_systemRegistry;

    public:
        Editor();

        void init() override;

        void start() override;

        void update() override;

        void fixedUpdate() override;

        void stop() override;

        void destroy() override;
    };
}
