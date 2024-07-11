#pragma once

#include <string>
#include <filesystem>

#include "Logger.hpp"


namespace TechEngine::FileSystem {
    inline std::filesystem::path rootPath = std::filesystem::current_path(); //This could be Editor path or Runtime path
    inline std::filesystem::path projectTemplate = rootPath.string() + "\\projectTemplate";
    inline std::filesystem::path scenesTemplate = projectTemplate.string() + "\\Resources\\Client\\defaults\\scenes\\DefaultScene.scene"; //TODO: rethink if this should take from project template or project resources
    inline std::filesystem::path scriptsTemplate = projectTemplate.string() + "\\Resources\\Common\\defaults\\scripts";
    inline std::filesystem::path clientPath = rootPath.string() + "\\TechEngineClientRuntime";
    inline std::filesystem::path serverPath = rootPath.string() + "\\TechEngineServerRuntime";
    inline std::filesystem::path editorResourcesPath = rootPath.string() + "\\resources"; //Only use this in editor

    inline std::vector<std::string> getAllFilesWithExtension(const std::vector<std::string>& paths, const std::vector<std::string>& extensions, bool recursive) {
        std::vector<std::string> filesWithExtension;
        for (const std::string& path: paths) {
            if (recursive) {
                for (const auto& entry: std::filesystem::recursive_directory_iterator(path)) {
                    if (std::find(extensions.begin(), extensions.end(), entry.path().extension().string()) != extensions.end()) {
                        filesWithExtension.push_back(entry.path().string());
                    }
                }
            } else {
                for (const auto& entry: std::filesystem::directory_iterator(path)) {
                    if (std::find(extensions.begin(), extensions.end(), entry.path().extension().string()) != extensions.end()) {
                        filesWithExtension.push_back(entry.path().string());
                    }
                }
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

    inline bool isExcluded(const std::filesystem::path& path, const std::vector<std::string>& excludedExtension, const std::vector<std::string>& excludedFolders) {
        return (std::find(excludedExtension.begin(), excludedExtension.end(), path.extension().string()) != excludedExtension.end()) ||
               (std::filesystem::is_directory(path) && std::find(excludedFolders.begin(), excludedFolders.end(), path.filename()) != excludedFolders.end());
    }

    inline void copyRecursive(const std::filesystem::path& source, const std::filesystem::path& destination, const std::vector<std::string>& excludedExtension, const std::vector<std::string>& excludedFolders) {
        for (const auto& entry: std::filesystem::directory_iterator(source)) {
            const std::filesystem::path& currentPath = entry.path();
            const std::filesystem::path destinationPath = destination / currentPath.filename();

            if (isExcluded(currentPath, excludedExtension, excludedFolders)) {
                continue;
            }

            if (std::filesystem::is_directory(currentPath)) {
                std::filesystem::create_directories(destinationPath);
                copyRecursive(currentPath, destinationPath, excludedExtension, excludedFolders);
            } else {
                std::filesystem::copy_file(currentPath, destinationPath);
            }
        }
    }
}
