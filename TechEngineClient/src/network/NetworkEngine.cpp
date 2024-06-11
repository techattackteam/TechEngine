#include "NetworkEngine.hpp"

#include "core/Logger.hpp"
#include "network/PacketType.hpp"
#include "serialization/BufferStream.hpp"

#include <steam/steamnetworkingsockets.h>
#include <steam/ISteamNetworkingUtils.h>
#include <WinSock2.h>
#include <ws2tcpip.h>

#include "events/network/ClientBanEvent.hpp"
#include "events/network/ClientKickEvent.hpp"
#include "events/network/ConnectionEstablishedEvent.hpp"
#include "events/network/CustomPacketReceived.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "network/SceneSynchronizer.hpp"
#include "scene/GameObject.hpp"

namespace TechEngine {
    NetworkEngine::NetworkEngine(EventDispatcher& eventDispatcher, SceneManager& sceneManager) : eventDispatcher(eventDispatcher), sceneManager(sceneManager), lastLoadedScene(sceneManager.getActiveSceneName()) {
    }

    NetworkEngine::~NetworkEngine() {
        if (running) {
            disconnectServer();
        }
    }


    std::string ResolveDomainName(std::string_view name) {
        // Adapted from example at https://learn.microsoft.com/en-us/windows/win32/api/ws2tcpip/nf-ws2tcpip-getaddrinfo

        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            printf("WSAStartup failed with %u\n", WSAGetLastError());
            return {};
        }

        addrinfo hints;
        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        addrinfo* addressResult = NULL;
        DWORD dwRetval = getaddrinfo(name.data(), nullptr, &hints, &addressResult);
        if (dwRetval != 0) {
            printf("getaddrinfo failed with error: %d\n", dwRetval);
            WSACleanup();
            return {};
        }

        std::string ipAddressStr;
        for (addrinfo* ptr = addressResult; ptr != NULL; ptr = ptr->ai_next) {
            switch (ptr->ai_family) {
                case AF_UNSPEC:
                    // Unspecified
                    break;
                case AF_INET: {
                    sockaddr_in* sockaddr_ipv4 = (sockaddr_in*)ptr->ai_addr;
                    char* ipAddress = inet_ntoa(sockaddr_ipv4->sin_addr);
                    ipAddressStr = ipAddress;
                    break;
                }
                case AF_INET6: {
                    const DWORD ipbufferlength = 46;
                    char ipstringbuffer[ipbufferlength];
                    DWORD actualIPBufferLength = ipbufferlength;
                    LPSOCKADDR sockaddr_ip = (LPSOCKADDR)ptr->ai_addr;
                    INT iRetval = WSAAddressToStringA(sockaddr_ip, (DWORD)ptr->ai_addrlen, nullptr, ipstringbuffer, &actualIPBufferLength);

                    if (iRetval) {
                        printf("WSAAddressToString failed with %u\n", WSAGetLastError());
                        WSACleanup();
                        return {};
                    }

                    ipAddressStr = std::string(ipstringbuffer, actualIPBufferLength);
                }
            }
        }

        freeaddrinfo(addressResult);
        WSACleanup();

        return ipAddressStr;
    }

    std::vector<std::string> SplitString(const std::string_view string, const std::string_view& delimiters) {
        size_t first = 0;

        std::vector<std::string> result;

        while (first <= string.size()) {
            const auto second = string.find_first_of(delimiters, first);

            if (first != second)
                result.emplace_back(string.substr(first, second - first));

            if (second == std::string_view::npos)
                break;

            first = second + 1;
        }

        return result;
    }

    std::vector<std::string> SplitString(const std::string_view string, const char delimiter) {
        return SplitString(string, std::string(1, delimiter));
    }

    void NetworkEngine::connectServer(const std::string& ip, const std::string& port) {
        connectionStatus = ConnectionStatus::Connecting;

        SteamDatagramErrMsg errMsg;
        if (!GameNetworkingSockets_Init(nullptr, errMsg)) {
            connectionDebugMessage = "Could not initialize GameNetworkingSockets";
            connectionStatus = ConnectionStatus::FailedToConnect;
            return;
        }

        // Select instance to use.  For now we'll always use the default.
        sockets = SteamNetworkingSockets();
        // Try resolve domain name
        std::string serverIP = ResolveDomainName(ip);
        if (port.empty())
            serverIP = fmt::format("{}:{}", serverIP, 8192); // Add default port if hostname doesn't contain port
        else
            serverIP = fmt::format("{}:{}", serverIP, port); // Add specified port
        // Start connecting
        SteamNetworkingIPAddr address;
        if (!address.ParseString(serverIP.c_str())) {
            TE_LOGGER_ERROR("Invalid IP address - could not parse {0}:{1}", ip, port);
            connectionDebugMessage = "Invalid IP address";
            connectionStatus = ConnectionStatus::FailedToConnect;
            return;
        }

        SteamNetworkingConfigValue_t options;
        options.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)onConnectionStatusChanged);
        connection = sockets->ConnectByIPAddress(address, 1, &options);
        if (connection == k_HSteamNetConnection_Invalid) {
            connectionDebugMessage = "Failed to create connection";
            connectionStatus = ConnectionStatus::FailedToConnect;
            return;
        }
        SteamNetworkingSockets()->SetConnectionUserData(connection, int64(static_cast<void*>(this)));
        running = true;
    }


    void NetworkEngine::update() {
        if (running) {
            pollIncomingMessages();
            pollConnectionStateChanges();
            //std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void NetworkEngine::fixedUpdate() {
    }

    void NetworkEngine::disconnectServer() {
        running = false;
        sockets->CloseConnection(connection, 0, nullptr, false);
        connection = k_HSteamNetConnection_Invalid;
        connectionStatus = ConnectionStatus::Disconnected;
        sceneManager.loadScene(lastLoadedScene);
    }

    void NetworkEngine::sendBuffer(Buffer buffer, bool reliable) {
        EResult result = sockets->SendMessageToConnection(connection, buffer.data, buffer.GetSize(), reliable ? k_nSteamNetworkingSend_Reliable : k_nSteamNetworkingSend_Unreliable, nullptr);
        if (result != k_EResultOK) {
            TE_LOGGER_ERROR("Failed to send message to server. Error: {0}", (int)result);
        } else {
            TE_LOGGER_INFO("Sent message to server");
        }
    }

    void NetworkEngine::sendString(const std::string& string, bool reliable) {
        sendBuffer(Buffer(string.c_str(), string.size()), reliable);
    }

    bool NetworkEngine::isRunning() {
        return running;
    }

    NetworkEngine::ConnectionStatus NetworkEngine::getConnectionStatus() const {
        return connectionStatus;
    }

    const std::string& NetworkEngine::getConnectionDebugMessage() const {
        return connectionDebugMessage;
    }

    void NetworkEngine::pollIncomingMessages() {
        while (running) {
            ISteamNetworkingMessage* incomingMessage = nullptr;
            int messageCount = sockets->ReceiveMessagesOnConnection(connection, &incomingMessage, 1);
            if (messageCount == 0)
                return;
            if (messageCount < 0) {
                // messageCount < 0 means critical error?
                running = false;
                return;
            }

            onDataReceived(Buffer(incomingMessage->m_pData, incomingMessage->m_cbSize));
            incomingMessage->Release();
        }
    }

    void NetworkEngine::pollConnectionStateChanges() {
        sockets->RunCallbacks();
    }

    void NetworkEngine::onConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info) {
        NetworkEngine* networkEngine = reinterpret_cast<NetworkEngine*>(SteamNetworkingSockets()->GetConnectionUserData(info->m_hConn));
        switch (info->m_info.m_eState) {
            case k_ESteamNetworkingConnectionState_None:
                // NOTE: We will get callbacks here when we destroy connections. You can ignore these.
                TE_LOGGER_INFO("Connection state changed to None");
                break;
            case k_ESteamNetworkingConnectionState_ClosedByPeer:
                TE_LOGGER_INFO("Disconnected from host. {0}", info->m_info.m_szEndDebug);
            case k_ESteamNetworkingConnectionState_ProblemDetectedLocally: {
                networkEngine->running = false;
                networkEngine->connectionStatus = ConnectionStatus::FailedToConnect;
                networkEngine->connectionDebugMessage = info->m_info.m_szEndDebug;

                // Print an appropriate message
                if (info->m_eOldState == k_ESteamNetworkingConnectionState_Connecting) {
                    // Note: we could distinguish between a timeout, a rejected connection,
                    // or some other transport problem.
                    TE_LOGGER_INFO("Could not connect to remote host. {0}", info->m_info.m_szEndDebug);
                } else if (info->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally) {
                    TE_LOGGER_INFO("Lost connection with remote host. {0}", info->m_info.m_szEndDebug);
                } else {
                    // NOTE: We could check the reason code for a normal disconnection
                    TE_LOGGER_INFO("Disconnected from host. {0}", info->m_info.m_szEndDebug);
                }

                // Clean up the connection.  This is important!
                // The connection is "closed" in the network sense, but
                // it has not been destroyed.  We must close it on our end, too
                // to finish up.  The reason information do not matter in this case,
                // and we cannot linger because it's already closed on the other end,
                // so we just pass 0s.

                networkEngine->disconnectServer();
                break;
            }

            case k_ESteamNetworkingConnectionState_Connecting:
                // We will get this callback when we start connecting.
                // We can ignore this.
                TE_LOGGER_INFO("Connecting to remote host");
                break;

            case k_ESteamNetworkingConnectionState_Connected:
                networkEngine->connectionStatus = ConnectionStatus::Connected;
                networkEngine->sendMessage("Hello from client");
                networkEngine->eventDispatcher.dispatch(new ConnectionEstablishedEvent());
                break;

            default:
                TE_LOGGER_INFO("Unhandled connection state change: {0}", (int)info->m_info.m_eState);
                break;
        }
    }

    void NetworkEngine::onDataReceived(Buffer buffer) {
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
                TE_LOGGER_INFO("Received message from server: {0}", message);
                break;
            }
            case PacketType::ClientConnectionRequest: {
                break;
            }
            case PacketType::ConnectionStatus: {
                break;
            }
            case PacketType::ClientList: {
                break;
            }
            case PacketType::ClientConnect: {
                break;
            }
            case PacketType::ClientUpdate: {
                break;
            }
            case PacketType::ClientDisconnect: {
                break;
            }
            case PacketType::ClientUpdateResponse: {
                break;
            }
            case PacketType::MessageHistory: {
                break;
            }
            case PacketType::ServerShutdown: {
                break;
            }
            case PacketType::ClientKick: {
                eventDispatcher.dispatch(new ClientKickEvent());
                break;
            }
            case PacketType::ClientBan: {
                eventDispatcher.dispatch(new ClientBanEvent());
                break;
            }
            case PacketType::SyncGameObject: {
                SceneSynchronizer::deserializeGameObject(stream, sceneManager.getScene());
                break;
            }
            case PacketType::SyncGameState: {
                sceneManager.saveCurrentScene();
                SceneSynchronizer::deserializeScene(stream, sceneManager);
                break;
            }
            case PacketType::CustomPacket: {
                std::string customPacket;
                stream.readString(customPacket);
                if (checkCustomPacketType(customPacket)) {
                    eventDispatcher.dispatch(new CustomPacketReceived(customPacket));
                }
            }

            default:
                break;
        }
    }

    void NetworkEngine::sendMessage(const std::string& message) {
        Buffer scratchBuffer;
        scratchBuffer.allocate(1024);
        BufferStreamWriter stream(scratchBuffer);
        stream.writeRaw<PacketType>(PacketType::Message);
        stream.writeString(message);
        sendBuffer(stream.getBuffer());
    }

    void NetworkEngine::sendCustomPacket(const std::string& packetType, Buffer buffer, bool reliable) {
        Buffer scratchBuffer;
        scratchBuffer.allocate(sizeof(PacketType::CustomPacket) + buffer.size);
        BufferStreamWriter stream(scratchBuffer);
        stream.writeRaw<PacketType>(PacketType::CustomPacket);
        stream.writeString(packetType);
        stream.writeBuffer(buffer);
        sendBuffer(stream.getBuffer(), reliable);
    }
}
