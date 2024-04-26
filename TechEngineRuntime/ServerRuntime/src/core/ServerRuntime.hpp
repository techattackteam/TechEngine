#pragma once

#include "core/Server.hpp"

namespace TechEngine {
    class ServerRuntime : public Server {
    private:

    public:
        ServerRuntime();

        ~ServerRuntime() override;

        void init();

        virtual void onUpdate();

        virtual void onFixedUpdate();
    };
}
