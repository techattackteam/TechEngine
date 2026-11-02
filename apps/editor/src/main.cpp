#include <TechEngine/app/EntryPoint.hpp>
#include <TechEngine/platform/Platform.hpp>

#include <EditorApp.hpp>

#include <filesystem>

int main(int argc, char** argv) {
    const std::filesystem::path projectRoot = argc > 1 ? argv[1] : TE_EDITOR_DEFAULT_PROJECT_ROOT;

    const std::filesystem::path executableFolder = TechEngine::executablePath().parent_path();

    return TechEngine::runApp<TechEngine::EditorApp>(projectRoot, executableFolder, executableFolder / TE_EDITOR_RUNTIME_FILE_NAME);
}
