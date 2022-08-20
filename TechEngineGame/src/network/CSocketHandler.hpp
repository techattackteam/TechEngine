#pragma once

#include <TechEngineCore.hpp>

namespace TechEngine {

    class CSocketHandler : public TechEngineCore::SocketHandler {
    private:
        udp::resolver *resolver;
    public:
        CSocketHandler(TechEngineCore::NetworkHandler *networkHandler);

        ~CSocketHandler();

        void init() override;

        udp::resolver *getResolver() const;
    };
}
