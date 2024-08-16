#include "Application.hpp"

namespace TechEngine {
    void Application::run() {
        int counter = 0;
        while (counter <= 10) {
            fixedUpdate();
            update();
            counter++;
        }
    }
}
