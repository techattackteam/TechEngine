#pragma once
#include "Panel.hpp"
#include "network/NetworkEngine.hpp"


namespace TechEngine {
    class NetworkHelper : public Panel {
    private:
        NetworkEngine& networkEngine;

    public:
        NetworkHelper(NetworkEngine& networkEngine, EventDispatcher& eventDispatcher);

        ~NetworkHelper() override;

        void onUpdate() override;
    };
}
