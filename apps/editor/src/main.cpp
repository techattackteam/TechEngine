#include <TechEngine/app/EntryPoint.hpp>

#include "EditorApp.hpp"

#include <filesystem>

int main(int argc, char** argv) {
    const std::filesystem::path projectRoot = argc > 1 ? argv[1] : TE_EDITOR_DEFAULT_PROJECT_ROOT;

    return TechEngine::runApp<TechEngine::EditorApp>(projectRoot);
}
