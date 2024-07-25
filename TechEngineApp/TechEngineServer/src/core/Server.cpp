#include "Server.hpp"
#include "components/render/MeshRendererComponent.hpp"
#include "core/FileSystem.hpp"
#include "core/Logger.hpp"
#include "core/Timer.hpp"
#include "events/connections/OnClientConnected.hpp"
#include "events/connections/OnClientDisconnected.hpp"
#include "events/network/CustomPacketReceived.hpp"
#include "network/PacketType.hpp"
#include "network/SceneSynchronizer.hpp"
#include "physics/PhysicsEngine.hpp"
#include "serialization/BufferStream.hpp"
#include "script/ScriptEngine.hpp"
#include "texture/TextureManager.hpp"
#include "core/UUID.hpp"
#include "network/NetworkObjectsManager.hpp"
#include "scriptingAPI/network/NetworkData.hpp"

namespace TechEngine {
    Server::Server() : project(systemsRegistry), m_communicator(*this), m_serverAPI(systemsRegistry, this, &m_communicator) {
        instance = this;
        systemsRegistry.registerSystem<NetworkObjectsManager>();
    }

    Server::~Server() {
        // Close all the connections
        TE_LOGGER_INFO("Closing connections...");
        for (const auto& [clientID, clientInfo]: m_connectedClients) {
            m_interface->CloseConnection(clientID, 0, "Server Shutdown", true);
        }

        m_connectedClients.clear();

        m_interface->CloseListenSocket(m_ListenSocket);
        m_ListenSocket = k_HSteamListenSocket_Invalid;

        m_interface->DestroyPollGroup(m_PollGroup);
        m_PollGroup = k_HSteamNetPollGroup_Invalid;
    }

    void Server::init() {
        AppCore::init();
        m_serverAPI.init();
        systemsRegistry.getSystem<Logger>().init("TechEngineServer");
        systemsRegistry.getSystem<PhysicsEngine>().init();
        ScriptRegister::getInstance()->init(&systemsRegistry.getSystem<ScriptEngine>());
        std::vector<std::string> paths = {
            project.getResourcesPath().string(),
            project.getCommonResourcesPath().string(),
            project.getAssetsPath().string(),
            project.getCommonAssetsPath().string()
        };
        systemsRegistry.getSystem<TextureManager>().init(FileSystem::getAllFilesWithExtension(paths, {".jpg", ".png"}, true));
        systemsRegistry.getSystem<MaterialManager>().init(FileSystem::getAllFilesWithExtension(paths, {".mat"}, true));
        systemsRegistry.getSystem<SceneManager>().init(FileSystem::getAllFilesWithExtension(paths, {".scene"}, true));
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
        NetworkData::networkID = 0;
    }

    void Server::onUpdate() {
        systemsRegistry.getSystem<Timer>().updateInterpolation();
        systemsRegistry.getSystem<Timer>().update();
        systemsRegistry.getSystem<Timer>().updateFPS();
    }

    void Server::onFixedUpdate() {
        systemsRegistry.getSystem<EventDispatcher>().fixedSyncEventManager.execute();
        systemsRegistry.getSystem<EventDispatcher>().syncEventManager.execute();
        systemsRegistry.getSystem<ScriptEngine>().onFixedUpdate();
        systemsRegistry.getSystem<ScriptEngine>().onUpdate();
        systemsRegistry.getSystem<SceneManager>().getScene().fixedUpdate();
        systemsRegistry.getSystem<SceneManager>().getScene().update();
        systemsRegistry.getSystem<NetworkObjectsManager>().fixedUpdate();
        systemsRegistry.getSystem<NetworkObjectsManager>().update();
        PollIncomingMessages();
        m_interface->RunCallbacks();
        syncGameObjects();
    }

    void Server::PollIncomingMessages() {
        while (running) {
            ISteamNetworkingMessage* incomingMessage = nullptr;
            int messageCount = m_interface->ReceiveMessagesOnPollGroup(m_PollGroup, &incomingMessage, 1);
            if (messageCount == 0)
                return;

            if (messageCount < 0) {
                // messageCount < 0 means critical error?
                running = false;
                return;
            }

            auto itClient = m_connectedClients.find(incomingMessage->m_conn);
            if (itClient == m_connectedClients.end()) {
                TE_LOGGER_ERROR("ERROR: Received data from unregistered client\n");
                continue;
            }

            if (incomingMessage->m_cbSize) {
                onDataReceivedCallback(itClient->second, Buffer(incomingMessage->m_pData, incomingMessage->m_cbSize));
            }

            incomingMessage->Release();
        }
    }

    void Server::syncGameObjects() {
        for (auto& gameObject: systemsRegistry.getSystem<SceneManager>().getScene().getGameObjects()) {
            if (gameObject->hasComponent<NetworkSync>()) {
                Buffer buffer = SceneSynchronizer::serializeGameObject(*gameObject);
                m_communicator.broadcastBuffer(buffer, true);
            }
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

                TE_LOGGER_INFO("Received message from {0}: {1}", clientInfo.ConnectionDesc, message);
                break;
            }
            case PacketType::NetworkIntSync: {
                unsigned int owner;
                std::string networkUUID;
                std::string name;
                int value;
                stream.readRaw<unsigned int>(owner);
                stream.readString(networkUUID);
                stream.readString(name);
                stream.readRaw<int>(value);
                Buffer scratchBuffer;
                scratchBuffer.allocate(sizeof(PacketType::NetworkIntSync) + sizeof(unsigned int) + (sizeof(char) * (name.size() + networkUUID.size())) + sizeof(int));
                BufferStreamWriter stream(scratchBuffer);
                stream.writeRaw<PacketType>(PacketType::NetworkIntSync);
                stream.writeRaw<unsigned int>(owner);
                stream.writeString(networkUUID);
                stream.writeString(name);
                stream.writeRaw<int>(value);
                m_communicator.broadcastBuffer(scratchBuffer, true);
                systemsRegistry.getSystem<EventDispatcher>().dispatch(new SyncNetworkInt(owner, networkUUID, name, value));
                break;
            }
            case PacketType::RequestNetworkObject: {
                std::string objectType;
                stream.readString(objectType);
                TE_LOGGER_INFO("Received request for network object: {0}", objectType);
                UUID uuid;
                std::string uuidString = std::to_string(uuid);
                NetworkObject* object = systemsRegistry.getSystem<NetworkObjectsManager>().createNetworkObject(objectType, clientInfo.networkID, uuidString);
                if (object != nullptr) {
                    m_communicator.sendNetworkObject(clientInfo, object);
                    TE_LOGGER_INFO("Created network object: {0} with UUID: {1}", objectType, uuidString);
                }
                break;
            }
            case PacketType::RequestDeleteNetworkObject: {
                std::string uuid;
                stream.readString(uuid);
                if (systemsRegistry.getSystem<NetworkObjectsManager>().deleteNetworkObject(uuid)) {
                    m_communicator.broadcastNetworkObjectDeleted(uuid);
                    TE_LOGGER_INFO("Deleted network object with UUID: {0}", uuid);
                } else {
                    TE_LOGGER_WARN("Failed to delete network object with UUID: {0}\n\tObject not found!", uuid);
                }
                break;
            }
            case PacketType::RequestNetworkVariable: {
                unsigned int owner;
                std::string uuid;
                std::string variableName;
                int value;
                stream.readRaw<unsigned int>(owner);
                stream.readString(uuid);
                stream.readString(variableName);
                stream.readRaw<int>(value);
                if (systemsRegistry.getSystem<NetworkObjectsManager>().registerNetworkVariable(owner, uuid, variableName, value)) {
                    m_communicator.broadcastNetworkVariable(owner, uuid, variableName, value);
                } else {
                    TE_LOGGER_WARN("Failed to register network variable with UUID: {0}\n\tObject not found!", uuid);
                }
                break;
            }
            case PacketType::CustomPacket: {
                std::string customPacket;
                stream.readString(customPacket);
                if (checkCustomPacketType(customPacket)) {
                    systemsRegistry.getSystem<EventDispatcher>().dispatch(new CustomPacketReceived(customPacket));
                }
                break;
            }
            default:
                break;
        }
    }

    void Server::onClientConnected(const ClientInfo& clientInfo) {
        m_communicator.sendNetworkID(clientInfo);
        m_communicator.syncGameState(clientInfo);
        m_communicator.syncNetworkObjects(clientInfo);
        systemsRegistry.getSystem<EventDispatcher>().dispatch(new OnClientConnected(clientInfo.internalID));
    }

    void Server::onClientDisconnected(const ClientInfo& clientInfo) {
        m_communicator.deleteAllNetworkObjectsFromClient(clientInfo);
        m_availableNetworkIDs.push(clientInfo.networkID);
        systemsRegistry.getSystem<EventDispatcher>().dispatch(new OnClientDisconnected(clientInfo.internalID));
    }

    int Server::getNextAvailableNetworkID() {
        if (m_availableNetworkIDs.empty()) {
            return m_nextNetworkID++;
        }

        int networkID = m_availableNetworkIDs.top();
        m_availableNetworkIDs.pop();
        return networkID;
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
                    auto itClient = m_connectedClients.find(status->m_hConn);
                    //assert(itClient != m_mapClients.end());

                    // Either ClosedByPeer or ProblemDetectedLocally - should be communicated to user callback
                    // User callback
                    onClientDisconnected(itClient->second);
                    m_connectedClients.erase(itClient);
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
                ClientInfo& client = m_connectedClients[status->m_hConn];
                client.internalID = status->m_hConn;
                client.networkID = getNextAvailableNetworkID();
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
