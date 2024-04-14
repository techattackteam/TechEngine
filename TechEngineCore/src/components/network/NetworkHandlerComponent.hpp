#pragma once

#include <filesystem>
#include <GameNetworkingSockets/steam/isteamnetworkingsockets.h>

#include "components/Component.hpp"
#include "serialization/Buffer.hpp"

namespace TechEngine {
    class NetworkHandlerComponent : public Component {
    public:
        enum class ConnectionStatus {
            Disconnected = 0, Connected, Connecting, FailedToConnect
        };

    private:
        ConnectionStatus connectionStatus = ConnectionStatus::Disconnected;
        std::string connectionDebugMessage;

        std::string serverAddress;
        bool running = false;

        ISteamNetworkingSockets* sockets = nullptr;
        HSteamNetConnection connection = 0;

    public:
        explicit NetworkHandlerComponent(GameObject* gameObject);

        ~NetworkHandlerComponent() override;

        void connectServer();

        void update() override;

        void fixedUpdate() override;

        void disconnectServer();

        void setServerAddress(const std::string& address);

        void sendBuffer(Buffer buffer, bool reliable = true);

        void sendString(const std::string& string, bool reliable = true);

        template<typename T>
        void sendData(const T& data, bool reliable = true) {
            sendBuffer(Buffer(&data, sizeof(T)), reliable);
        }

        bool isRunning();

        ConnectionStatus getConnectionStatus() const;

        const std::string& getConnectionDebugMessage() const;

        const std::string& getServerAddress() const;

        Component* copy(GameObject* gameObjectToAttach, Component* componentToCopy) override;

    private:
        void pollIncomingMessages();

        void pollConnectionStateChanges();

        static void onConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);

        void sendMessage(const std::string& message);
    };
}
