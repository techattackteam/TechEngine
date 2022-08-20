#include "Server.hpp"


Server::Server() {
}

Server::~Server() {

}

void Server::run() {
    App::run();
}

void Server::onUpdate() {

}

void Server::onFixedUpdate() {

}

TechEngineCore::App *TechEngineCore::createApp() {
    return new Server();
}
