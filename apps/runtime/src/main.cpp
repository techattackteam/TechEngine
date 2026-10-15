#include <TechEngine/app/EntryPoint.hpp>
#include <TechEngine/platform/Platform.hpp>

#include <RuntimeApp.hpp>

int main() {
    // TODO: reads the dev project's first export (bin/<config>/Dev) until the export command
    // decides where a target's packs land.
    return TechEngine::runApp<TechEngine::RuntimeApp>(TechEngine::executablePath().parent_path() / "Dev");
}
