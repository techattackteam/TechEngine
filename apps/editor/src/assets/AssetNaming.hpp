#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <system_error>

namespace TechEngine {
    class AssetNaming {
    public:
        // False for an empty name, a path separator, a character Windows refuses in a file name, or a trailing dot or
        // space, which Windows drops.
        static bool isValidName(std::string_view name);

        // The last part of path without its extension. A leading dot starts no extension, so .hidden stays whole.
        static std::string_view stemOf(std::string_view path);

        // The path up to its last '/', without it. Empty when path has none.
        static std::string_view folderOf(std::string_view path);

        // folder + "/" + name, or name alone when folder is empty.
        static std::string join(std::string_view folder, std::string_view name);

        // A suffix of 0 adds nothing, and 1 turns Brick into Brick_1.
        static std::string withSuffix(std::string_view name, std::uint32_t suffix);

        // Calls create on candidate(0), candidate(1), ... until one is not AlreadyExists, and puts that path in out when it
        // succeeded. A name is taken by creating it, so two creates at once never share one, even from two processes.
        static std::error_code claimFirstFree(const std::function<std::string(std::uint32_t suffix)>& candidate, const std::function<std::error_code(const std::string& path)>& create, std::string& out);
    };
}
