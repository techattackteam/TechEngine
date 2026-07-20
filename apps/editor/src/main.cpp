#include <TechEngine/app/App.hpp>
#include <iostream>

// Thin entry point — the editor shell and asset pipeline land here later
// (ADR-006 §1). For now it exercises the same app entry as runtime.
int main() {
    std::cout << "Hello World" << std::endl;
    return TechEngine::run();
}
