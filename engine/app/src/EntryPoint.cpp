#include <Application.hpp>
#include <cstdlib>

int main(int argc, char** argv) {
    auto app = TechEngine::createApp();
    app->init();
    app->start();
    app->run();
    app->stop();
    app->destroy();
    delete app;
    exit(0);
}
