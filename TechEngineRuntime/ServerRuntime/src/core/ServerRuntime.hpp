#pragma once

#include "core/Server.hpp"

namespace TechEngine {
    class ServerRuntime : public Server {
    public:
        ServerRuntime();

        void init();

    private:
        void onUpdate() override;

        void onFixedUpdate() override;
    };
}
