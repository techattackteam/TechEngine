#include <Application.hpp>
#include <cstdlib>

int main(int argc, char** argv) {
    auto app = TechEngine::createApp();
    app->init();
    app->start();
    app->run();
    app->stop();
    app->shutdown();
    //delete app; //This is making in a loop
    exit(0);
}
