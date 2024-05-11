#pragma once
#include <map>

#include "core/Core.hpp"
#include "core/FilePaths.hpp"
#include "core/AppCore.hpp"
#include "serialization/Buffer.hpp"
#include "ClientInfo.hpp"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

#include "communicator/Communicator.hpp"
#include "scriptingAPI/TechEngineServerAPI.hpp"
#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <steam/steam_api.h>
#endif

namespace TechEngine {
    class TECHENGINE_API Server : public AppCore {
    public:
        FilePaths filePaths;

    protected:
        inline static Server* instance;

        std::thread networkThread;
        int m_port = 0;
        std::map<HSteamNetConnection, ClientInfo> m_ConnectedClients;

        ISteamNetworkingSockets* m_interface = nullptr;
        HSteamListenSocket m_ListenSocket = 0u;
        HSteamNetPollGroup m_PollGroup = 0u;
        Communicator m_Communicator;

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
    };
}
