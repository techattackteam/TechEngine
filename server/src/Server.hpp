#pragma once

#include <TechEngineServer.hpp>
#include <EntryPoint.hpp>

class Server : public TechEngineServer::App {
private:
    bool running;

public:
    Server();

    ~Server();

    void run() override;

    void onUpdate() override;

    void onFixedUpdate() override;


};
