#pragma once
#include <map>

#include "core/Core.hpp"
#include "core/AppCore.hpp"
#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

#include "serialization/Buffer.hpp"
#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <steam/steam_api.h>
#endif

namespace TechEngine {
    class TECHENGINE_API Server : public AppCore {
    private:
        inline static Server* instance;
        using ClientID = HSteamNetConnection;

        struct ClientInfo {
            ClientID ID;
            std::string ConnectionDesc;
        };

        std::thread networkThread;
        int m_port = 0;
        std::map<HSteamNetConnection, ClientInfo> m_ConnectedClients;

        std::function<void(const ClientInfo&)> m_ClientConnectedCallback;
        std::function<void(const ClientInfo&)> m_ClientDisconnectedCallback;

        ISteamNetworkingSockets* m_interface = nullptr;
        HSteamListenSocket m_ListenSocket = 0u;
        HSteamNetPollGroup m_PollGroup = 0u;

    public:
        Server();

        ~Server() override;

        void run() override;

        virtual void onUpdate();

        virtual void onFixedUpdate();

        void setClientConnectedCallback(const std::function<void(const ClientInfo&)>& function);

        void setClientDisconnectedCallback(std::function<void(const ClientInfo&)>& function);

        void sendBufferToClient(ClientID clientID, Buffer buffer, bool reliable);

        void sendBufferToAllClients(Buffer buffer, ClientID excludeClientID, bool reliable);

        void sendStringToClient(ClientID clientID, const std::string& string, bool reliable);

        void sendStringToAllClients(const std::string& string, ClientID excludeClientID, bool reliable);

        template<typename T>
        void sendDataToClient(ClientID clientID, const T& data, bool reliable = true) {
            sendBufferToClient(clientID, Buffer(&data, sizeof(T)), reliable);
        }

        template<typename T>
        void sendDataToAllClients(const T& data, ClientID excludeClientID = 0, bool reliable = true) {
            sendBufferToAllClients(Buffer(&data, sizeof(T)), excludeClientID, reliable);
        }

        void KickClient(ClientID clientID);

    private:
        static void ConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* info);

        void onConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);

        void setClientNick(HSteamNetConnection hConnection, const char* nick);

        void PollIncomingMessages();

        void onDataReceivedCallback(const ClientInfo&, const Buffer);
    };
}
