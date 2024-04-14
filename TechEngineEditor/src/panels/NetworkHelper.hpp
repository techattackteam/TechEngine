#pragma once
#include "Panel.hpp"
#include "network/NetworkEngine.hpp"


namespace TechEngine {
    class NetworkHelper : public Panel {
    private:
        NetworkEngine& networkEngine;

    public:
        NetworkHelper(NetworkEngine& networkEngine);

        ~NetworkHelper() override;

        void onUpdate() override;
    };
}
