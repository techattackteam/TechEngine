#pragma once

#include <string>
#include <filesystem>


namespace TechEngine::FileSystem {
    inline std::filesystem::path rootPath = std::filesystem::current_path(); //This could be project path or Runtime path
    inline std::filesystem::path projectTemplate = rootPath.string() + "\\projectTemplate";
    inline std::filesystem::path scenesTemplate = projectTemplate.string() + "\\scenes\\DefaultScene.scene";
    inline std::filesystem::path scriptsTemplate = projectTemplate.string() + "\\scripts";

    inline std::vector<std::string> getAllFilesWithExtension(const std::string &extension) {
        std::vector<std::string> filesWithExtension;
        for (const auto &entry: std::filesystem::recursive_directory_iterator(rootPath)) {
            if (entry.path().extension() == extension) {
                filesWithExtension.push_back(entry.path().string());
            }
        }
        return filesWithExtension;
    }

    inline std::string getFileName(std::string filepath) {
        std::filesystem::path path = filepath;
        std::string fileName = path.filename().string();
        std::string name = fileName.substr(0, fileName.find_last_of('.'));
        return name;
    }
}
