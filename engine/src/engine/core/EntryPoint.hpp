#pragma once

#include "App.hpp"

extern Engine::App *Engine::createApp();


int main(int argc, char **argv) {
    auto app = Engine::createApp();

    app->run();

    delete app;
}
