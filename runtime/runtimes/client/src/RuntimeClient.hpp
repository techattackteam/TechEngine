#pragma once
#include "Application.hpp"
#include "core/Client.hpp"

namespace TechEngine {
    class RuntimeClient : public Application {
        Client client;

    public:
        void init() override;

        void fixedUpdate() override;

        void update() override;

        void destroy() override;
    };
}
