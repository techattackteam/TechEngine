#pragma once

#include "core/App.hpp"

extern TechEngineCore::App *TechEngineCore::createApp();

int main(int argc, char **argv) {
    auto app = TechEngineCore::createApp();
    app->run();
    delete app;
    exit(0);
}