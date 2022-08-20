
#include "../core/App.hpp"

#pragma once

extern TechEngineCore::App *TechEngineCore::createApp();

int main(int argc, char **argv) {
    auto app = TechEngineCore::createApp();
    app->run();
    delete app;
    exit(0);
}