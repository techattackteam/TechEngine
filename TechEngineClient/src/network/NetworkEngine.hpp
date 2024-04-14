#pragma once
#include "serialization/Buffer.hpp"

#include <filesystem>
#include <steam/isteamnetworkingsockets.h>

#include "scene/Scene.hpp"

namespace TechEngine {
    class NetworkEngine {
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

        Scene& scene;

    public:
        explicit NetworkEngine(Scene& scene);

        ~NetworkEngine();

        void connectServer();

        void update();

        void fixedUpdate();

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

    private:
        void pollIncomingMessages();

        void pollConnectionStateChanges();

        static void onConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);

        void onDataReceived(Buffer buffer);

        void sendMessage(const std::string& message);
    };
}
