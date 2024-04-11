#pragma once

#include "core/AppCore.hpp"
#include "core/Logger.hpp"

extern TechEngine::AppCore* TechEngine::createApp();

int main(int argc, char** argv) {
    TechEngine::Logger::init();
    auto app = TechEngine::createApp();
    app->run();
    delete app;
    exit(0);
}
