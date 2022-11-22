#pragma once

#include "core/App.hpp"
#include "core/Logger.hpp"

extern TechEngineCore::App *TechEngineCore::createApp();

int main(int argc, char **argv) {
    TechEngine::Logger::init();
    auto app = TechEngineCore::createApp();
    app->run();
    delete app;
    exit(0);
}