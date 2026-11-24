#include <TechEngine/platform/files/FileError.hpp>

#include <assets/index/AssetNaming.hpp>

#include <algorithm>
#include <utility>

namespace TechEngine {
    static constexpr std::string_view REFUSED_NAME_CHARACTERS = "/\\:*?\"<>|";

    bool AssetNaming::isValidName(const std::string_view name) {
        if (name.empty() || name.back() == '.' || name.back() == ' ') {
            return false;
        }
        return std::ranges::none_of(name, [](const char character) {
            return static_cast<unsigned char>(character) < 0x20 || REFUSED_NAME_CHARACTERS.find(character) != std::string_view::npos;
        });
    }

    std::string_view AssetNaming::stemOf(const std::string_view path) {
        const std::size_t slash = path.find_last_of('/');
        const std::string_view fileName = slash == std::string_view::npos ? path : path.substr(slash + 1);
        const std::size_t dot = fileName.find_last_of('.');
        if (dot == std::string_view::npos || dot == 0) {
            return fileName;
        }
        return fileName.substr(0, dot);
    }

    std::string_view AssetNaming::folderOf(const std::string_view path) {
        const std::size_t slash = path.find_last_of('/');
        if (slash == std::string_view::npos) {
            return {};
        }
        return path.substr(0, slash);
    }

    std::string AssetNaming::join(const std::string_view folder, const std::string_view name) {
        if (folder.empty()) {
            return std::string(name);
        }
        return std::string(folder) + "/" + std::string(name);
    }

    std::string AssetNaming::withSuffix(const std::string_view name, const std::uint32_t suffix) {
        if (suffix == 0) {
            return std::string(name);
        }
        return std::string(name) + "_" + std::to_string(suffix);
    }

    std::error_code AssetNaming::claimFirstFree(const std::function<std::string(std::uint32_t suffix)>& candidate, const std::function<std::error_code(const std::string& path)>& create, std::string& out) {
        std::error_code createError;
        std::uint32_t suffix = 0;
        do {
            std::string path = candidate(suffix++);
            createError = create(path);
            if (!createError) {
                out = std::move(path);
            }
        } while (createError == FileError::AlreadyExists);
        return createError;
    }
}
