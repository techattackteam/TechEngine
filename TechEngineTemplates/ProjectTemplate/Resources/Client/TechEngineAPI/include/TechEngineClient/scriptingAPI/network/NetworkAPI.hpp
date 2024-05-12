#pragma once
#include <string>

namespace TechEngine {
    class NetworkEngine;

    class NetworkAPI {
    private:
        inline static NetworkEngine* networkEngine;

    public:
        explicit NetworkAPI(NetworkEngine* networkEngine);

        ~NetworkAPI() = default;

        static void connectToServer(const std::string& ip, const std::string& port);

        static void disconnectFromServer();
    };
}
