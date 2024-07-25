#pragma once
#include <map>
#include <stack>

#include "core/ServerExportDll.hpp"
#include "core/AppCore.hpp"
#include "serialization/Buffer.hpp"
#include "ClientInfo.hpp"
#include "communicator/Communicator.hpp"
#include "project/Project.hpp"
#include "scriptingAPI/TechEngineServerAPI.hpp"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

#include "network/NetworkObjectsRegistry.hpp"
#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <steam/steam_api.h>
#endif


namespace TechEngine {
    class SERVER_DLL Server : public AppCore {
    public:
        Project project;
        NetworkObjectsRegistry networkObjectsRegistry;

    protected:
        inline static Server* instance;

        //std::thread networkThread;
        int m_port = 0;
        std::map<ClientID, ClientInfo> m_connectedClients;
        std::stack<int> m_availableNetworkIDs;
        int m_nextNetworkID = 1;

        ISteamNetworkingSockets* m_interface = nullptr;
        HSteamListenSocket m_ListenSocket = 0u;
        HSteamNetPollGroup m_PollGroup = 0u;
        Communicator m_communicator;

        friend class Communicator;
        friend class ServerAPI;

    private:
        TechEngineServerAPI m_serverAPI;

    public:
        Server();

        virtual ~Server();

        void init();

        virtual void onUpdate();

        virtual void onFixedUpdate();

        void kickClient(ClientID clientID);

    private:
        static void connectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* info);

        void onConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);

        void setClientNick(HSteamNetConnection hConnection, const char* nick);

        void PollIncomingMessages();

        void syncGameObjects();

        void onDataReceivedCallback(const ClientInfo&, Buffer);

        void onClientConnected(const ClientInfo& clientInfo);

        void onClientDisconnected(const ClientInfo& clientInfo);

        int getNextAvailableNetworkID();
    };
}
