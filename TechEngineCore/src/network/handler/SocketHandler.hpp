#pragma once

#include <boost/asio/ip/udp.hpp>
#include <thread>
#include "../Packet.hpp"

#include "NetworkHandler.hpp"

using boost::asio::ip::udp;

#define MAX_PACKET_SIZE 100000

namespace TechEngine {
    class NetworkHandler;

    class SocketHandler {
    private:
        boost::system::error_code error;

        std::thread *receiveThread;
        std::thread *sendThread;

        bool readFromSocket(std::string *buffer, udp::endpoint *senderEndpoint, int *byte);

        bool writeToSocket(Packet *packet);

        void read();

        void write();


    protected:
        NetworkHandler *networkHandler;
        boost::asio::io_context context;

        explicit SocketHandler(NetworkHandler *networkHandler);

    public:
        udp::socket *socket;

        std::string serializePacket(Packet *packet);

        Packet *deserializePacket(std::string buffer);

        virtual ~SocketHandler();

        virtual void init();

    };
}
