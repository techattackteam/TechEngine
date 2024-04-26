#pragma once
#include <map>

#include "core/Core.hpp"
#include "core/AppCore.hpp"
#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

#include "core/FilePaths.hpp"
#include "serialization/Buffer.hpp"
#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <steam/steam_api.h>
#endif

namespace TechEngine {
    class TECHENGINE_API Server : public AppCore {
    public:
        FilePaths filePaths;

    protected:
        inline static Server* instance;
        using ClientID = HSteamNetConnection;

        struct ClientInfo {
            ClientID ID;
            std::string ConnectionDesc;
        };

        std::thread networkThread;
        int m_port = 0;
        std::map<HSteamNetConnection, ClientInfo> m_ConnectedClients;

        ISteamNetworkingSockets* m_interface = nullptr;
        HSteamListenSocket m_ListenSocket = 0u;
        HSteamNetPollGroup m_PollGroup = 0u;

    public:
        Server();

        virtual ~Server();

        void init();

        virtual void onUpdate();

        virtual void onFixedUpdate();

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

        void onDataReceivedCallback(const ClientInfo&, Buffer);

        void onClientConnected(const ClientInfo& clientInfo);

        void syncGameState(const ClientInfo& clientInfo);
    };
}
