#pragma once
#include "core/AppCore.hpp"


namespace TechEngine {
    class TECHENGINE_API Server : public AppCore {
    public:
        Server();

        ~Server() override;

        void run() override;

        virtual void onUpdate();

        virtual void onFixedUpdate();

    private:
    };
}
