#include <TechEngine/app/EntryPoint.hpp>
#include <TechEngine/platform/Platform.hpp>

#include <RuntimeApp.hpp>

int main() {
    return TechEngine::runApp<TechEngine::RuntimeApp>(TechEngine::executablePath().parent_path());
}
