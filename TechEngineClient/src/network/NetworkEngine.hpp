#pragma once

#include "serialization/Buffer.hpp"
#include "scene/SceneManager.hpp"

#include <steam/isteamnetworkingsockets.h>
#include <filesystem>

namespace TechEngine {
    class NetworkEngine {
    public:
        enum class ConnectionStatus {
            Disconnected = 0, Connected, Connecting, FailedToConnect
        };

    private:
        EventDispatcher& eventDispatcher;
        ConnectionStatus connectionStatus = ConnectionStatus::Disconnected;
        std::string connectionDebugMessage;

        //std::string serverAddress;
        bool running = false;

        ISteamNetworkingSockets* sockets = nullptr;
        HSteamNetConnection connection = 0;

        SceneManager& sceneManager;
        const std::string& lastLoadedScene;

    public:
        explicit NetworkEngine(EventDispatcher& eventDispatcher, SceneManager& sceneManager);

        ~NetworkEngine();

        void connectServer(const std::string& ip, const std::string& port);

        void update();

        void fixedUpdate();

        void disconnectServer();

        void sendBuffer(Buffer buffer, bool reliable = true);

        void sendString(const std::string& string, bool reliable = true);

        template<typename T>
        void sendData(const T& data, bool reliable = true) {
            sendBuffer(Buffer(&data, sizeof(T)), reliable);
        }

        bool isRunning();

        ConnectionStatus getConnectionStatus() const;

        const std::string& getConnectionDebugMessage() const;

    private:
        void pollIncomingMessages();

        void pollConnectionStateChanges();

        static void onConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);

        void onDataReceived(Buffer buffer);

        void sendMessage(const std::string& message);
    };
}
