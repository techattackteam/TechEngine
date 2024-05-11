#include "Server.hpp"
#include "components/render/MeshRendererComponent.hpp"
#include "events/connections/OnClientConnected.hpp"
#include "events/connections/OnClientConnectionRequest.hpp"
#include "events/connections/OnClientDisconnected.hpp"
#include "events/network/CustomPacketReceived.hpp"
#include "external/EntryPoint.hpp"
#include "network/PacketType.hpp"
#include "network/SceneSynchronizer.hpp"
#include "serialization/BufferStream.hpp"
#include "script/ScriptEngine.hpp"

namespace TechEngine {
    Server::Server() : m_Communicator(*this), m_serverAPI(this, &m_Communicator) {
        instance = this;
        physicsEngine.init();
    }

    Server::~Server() {
        // Close all the connections
        TE_LOGGER_INFO("Closing connections...");
        for (const auto& [clientID, clientInfo]: m_ConnectedClients) {
            m_interface->CloseConnection(clientID, 0, "Server Shutdown", true);
        }

        m_ConnectedClients.clear();

        m_interface->CloseListenSocket(m_ListenSocket);
        m_ListenSocket = k_HSteamListenSocket_Invalid;

        m_interface->DestroyPollGroup(m_PollGroup);
        m_PollGroup = k_HSteamNetPollGroup_Invalid;
    }

    void Server::init() {
        running = true;
        m_port = 25565;
        SteamDatagramErrMsg errMsg;
        if (!GameNetworkingSockets_Init(nullptr, errMsg)) {
            TE_LOGGER_CRITICAL("GameNetworkingSockets_Init failed: {0}", errMsg);
            return;
        }

        m_interface = SteamNetworkingSockets();

        // Start listening
        SteamNetworkingIPAddr serverLocalAddress;
        serverLocalAddress.Clear();
        serverLocalAddress.m_port = m_port;

        SteamNetworkingConfigValue_t options;
        options.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)connectionStatusChangedCallback);

        // Try to start listen socket on port
        m_ListenSocket = m_interface->CreateListenSocketIP(serverLocalAddress, 1, &options);
        if (m_ListenSocket == k_HSteamListenSocket_Invalid) {
            TE_LOGGER_CRITICAL("Fatal error: Failed to listen on port {0}", m_port);
            return;
        }

        // Try to create poll group
        m_PollGroup = m_interface->CreatePollGroup();
        if (m_PollGroup == k_HSteamNetPollGroup_Invalid) {
            TE_LOGGER_CRITICAL("Fatal error: Failed to listen on port {0}", m_port);
            return;
        }
    }

    void Server::onUpdate() {
        timer.updateInterpolation();
        timer.update();
        timer.updateFPS();
    }

    void Server::onFixedUpdate() {
        eventDispatcher.fixedSyncEventManager.execute();
        eventDispatcher.syncEventManager.execute();
        scriptEngine.onFixedUpdate();
        scriptEngine.onUpdate();
        sceneManager.getScene().fixedUpdate();
        sceneManager.getScene().update();
        PollIncomingMessages();
        m_interface->RunCallbacks();
        syncGameObjects();
    }

    void Server::PollIncomingMessages() {
        while (running) {
            ISteamNetworkingMessage* incomingMessage = nullptr;
            int messageCount = m_interface->ReceiveMessagesOnPollGroup(m_PollGroup, &incomingMessage, 1);
            if (messageCount == 0)
                break;

            if (messageCount < 0) {
                // messageCount < 0 means critical error?
                running = false;
                return;
            }

            auto itClient = m_ConnectedClients.find(incomingMessage->m_conn);
            if (itClient == m_ConnectedClients.end()) {
                TE_LOGGER_ERROR("ERROR: Received data from unregistered client\n");
                continue;
            }

            if (incomingMessage->m_cbSize)
                onDataReceivedCallback(itClient->second, Buffer(incomingMessage->m_pData, incomingMessage->m_cbSize));

            incomingMessage->Release();
        }
    }

    void Server::syncGameObjects() {
        for (auto& gameObject: sceneManager.getScene().getGameObjects()) {
            Buffer buffer = SceneSynchronizer::serializeGameObject(*gameObject);
            m_Communicator.sendBufferToAllClients(buffer, 0, true);
        }
    }

    void Server::onDataReceivedCallback(const ClientInfo& clientInfo, Buffer buffer) {
        BufferStreamReader stream(buffer);

        PacketType type;
        bool success = stream.readRaw<PacketType>(type);
        if (!success) // Why couldn't we read packet type? Probs invalid packet
            return;

        switch (type) {
            case PacketType::Message: {
                std::string message;
                success = stream.readString(message);
                if (!success)
                    return;

                // User callback
                TE_LOGGER_INFO("Received message from {0}: {1}", clientInfo.ConnectionDesc, message);
                break;
            }
            case PacketType::ClientConnectionRequest: {
                eventDispatcher.dispatch(new OnClientConnectionRequest());
                break;
            }
            case PacketType::CustomPacket: {
                std::string customPacket;
                stream.readString(customPacket);
                if (checkCustomPacketType(customPacket)) {
                    eventDispatcher.dispatch(new CustomPacketReceived(customPacket));
                }
                break;
            }
            default:
                break;
        }
    }

    void Server::onClientConnected(const ClientInfo& clientInfo) {
        m_Communicator.syncGameState(clientInfo);
        eventDispatcher.dispatch(new OnClientConnected(clientInfo.ID));
    }

    void Server::connectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* info) {
        instance->onConnectionStatusChanged(info);
    }

    void Server::onConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* status) {
        // Handle connection state
        switch (status->m_info.m_eState) {
            case k_ESteamNetworkingConnectionState_None:
                // NOTE: We will get callbacks here when we destroy connections.  You can ignore these.
                break;
            case k_ESteamNetworkingConnectionState_ClosedByPeer:
            case k_ESteamNetworkingConnectionState_ProblemDetectedLocally: {
                // Ignore if they were not previously connected.  (If they disconnected
                // before we accepted the connection.)
                if (status->m_eOldState == k_ESteamNetworkingConnectionState_Connected) {
                    // Locate the client.  Note that it should have been found, because this
                    // is the only codepath where we remove clients (except on shutdown),
                    // and connection change callbacks are dispatched in queue order.
                    auto itClient = m_ConnectedClients.find(status->m_hConn);
                    //assert(itClient != m_mapClients.end());

                    // Either ClosedByPeer or ProblemDetectedLocally - should be communicated to user callback
                    // User callback
                    m_ConnectedClients.erase(itClient);
                } else {
                    //assert(info->m_eOldState == k_ESteamNetworkingConnectionState_Connecting);
                }

                // Clean up the connection.  This is important!
                // The connection is "closed" in the network sense, but
                // it has not been destroyed.  We must close it on our end, too
                // to finish up.  The reason information do not matter in this case,
                // and we cannot linger because it's already closed on the other end,
                // so we just pass 0s.
                TE_LOGGER_INFO("Client disconnected: {0}", status->m_info.m_szEndDebug);
                m_interface->CloseConnection(status->m_hConn, 0, nullptr, false);
                eventDispatcher.dispatch(new OnClientDisconnected());
                break;
            }

            case k_ESteamNetworkingConnectionState_Connecting: {
                // This must be a new connection
                // assert(m_mapClients.find(info->m_hConn) == m_mapClients.end());

                // Try to accept incoming connection
                if (m_interface->AcceptConnection(status->m_hConn) != k_EResultOK) {
                    m_interface->CloseConnection(status->m_hConn, 0, nullptr, false);
                    TE_LOGGER_INFO("Couldn't accept connection: {0}", status->m_info.m_szEndDebug);
                    break;
                }

                // Assign the poll group
                if (!m_interface->SetConnectionPollGroup(status->m_hConn, m_PollGroup)) {
                    m_interface->CloseConnection(status->m_hConn, 0, nullptr, false);
                    TE_LOGGER_INFO("Failed to set poll group");
                    break;
                }

                // Retrieve connection info
                SteamNetConnectionInfo_t connectionInfo;
                m_interface->GetConnectionInfo(status->m_hConn, &connectionInfo);

                // Register connected client
                auto& client = m_ConnectedClients[status->m_hConn];
                client.ID = (ClientID)status->m_hConn;
                client.ConnectionDesc = connectionInfo.m_szConnectionDescription;

                // User callback
                onClientConnected(client);
                TE_LOGGER_INFO("Client connected: {0}", client.ConnectionDesc);
                break;
            }

            case k_ESteamNetworkingConnectionState_Connected:
                // We will get a callback immediately after accepting the connection.
                // Since we are the server, we can ignore this, it's not news to us.
                break;

            default:
                break;
        }
    }

    void Server::setClientNick(HSteamNetConnection hConnection, const char* nick) {
        m_interface->SetConnectionName(hConnection, nick);
    }

    void Server::kickClient(ClientID clientID) {
        m_interface->CloseConnection(clientID, 0, "Kicked by host", false);
    }
}

TechEngine::AppCore* TechEngine::createApp() {
    return nullptr;
}
