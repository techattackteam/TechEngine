#pragma once
#include <functional>
#include <string>
#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

#include "serialization/Buffer.hpp"

namespace TechEngine {
    class Client {
    private:
        std::function<void(Buffer)> dataReceivedCallback;
        std::function<void()> serverConnectedCallback;
        std::function<void()> serverDisconnectedCallback;

        //ConnectionStatus connectionStatus = ConnectionStatus::Disconnected;
        std::string connectionDebugMessage;

        std::string serverAddress;
        bool running = false;

        ISteamNetworkingSockets* m_Interface = nullptr;
        HSteamNetConnection m_Connection = 0;
    };
}
