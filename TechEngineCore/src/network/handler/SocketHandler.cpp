#include <iostream>
#include "SocketHandler.hpp"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

namespace TechEngine {
    SocketHandler::SocketHandler(NetworkHandler *networkHandler) : networkHandler(networkHandler) {
    }

    SocketHandler::~SocketHandler() {
        receiveThread->join();
        sendThread->join();
        socket->close();
        delete (socket);
        delete receiveThread;
        delete sendThread;
    }

    void SocketHandler::init() {
        receiveThread = new std::thread(&SocketHandler::read, this);
        sendThread = new std::thread(&SocketHandler::write, this);
    }

    bool SocketHandler::readFromSocket(std::string *string, udp::endpoint *senderEndpoint, int *bytes) {
        char buffer[MAX_PACKET_SIZE + 1];
        *bytes = socket->receive_from(boost::asio::buffer(buffer, MAX_PACKET_SIZE), *senderEndpoint, 0, error);
        if (!error && *bytes != 0) {
            *string = buffer;
            return true;
        } else {
            std::cout << "Failed to read from socket: " << error << std::endl;
            return false;
        }
    }

    void SocketHandler::read() {
        while (networkHandler->isRunning()) {
            Packet *packet = nullptr;
            int bytes;
            std::string buffer;
            udp::endpoint endpoint;
            if (!readFromSocket(&buffer, &endpoint, &bytes)) {
                continue;
            }

            packet = deserializePacket(buffer);
            if (packet == nullptr) {
                continue;
            }
            packet->bytes = bytes;
            packet->senderEndpoint = endpoint;
            /*Figure if this should be instantly call on packet received or be stored to be processed later*/
            packet->onPacketReceive();
            //networkHandler->getPacketHandler().storeIncomePacket(packet);

        }
    }

    bool SocketHandler::writeToSocket(Packet *packet) {
        packet->bytes = socket->send_to(boost::asio::buffer(packet->data, packet->data.length()), packet->senderEndpoint, 0, error);
        if (!error) {
            return true;
        } else {
            std::cout << "Failed to write to socket: " << error << std::endl;
            return false;
        }
    }

    void SocketHandler::write() {
        while (networkHandler->isRunning()) {
            Packet *packet = networkHandler->getPacketHandler().getNextOutcomePacket();
            if (packet == nullptr) {
                continue;
            }

            packet->data = serializePacket(packet);
            writeToSocket(packet);
            //TEMPORARY FIX
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            delete (packet);
        }
    }


    std::string SocketHandler::serializePacket(Packet *packet) {
        std::stringstream ss;
        boost::archive::text_oarchive oa{ss};
        oa << packet;
        return ss.str();
    }

    Packet *SocketHandler::deserializePacket(std::string buffer) {
        std::stringstream stringStream(buffer);
        boost::archive::text_iarchive ia{stringStream};
        Packet *packet;
        ia >> packet;
        return packet;
    }


}
