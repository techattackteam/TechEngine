#pragma once
#include <string>
#include <vector>

namespace TechEngine {
    class IResourceLoader {
    public:
        virtual ~IResourceLoader() = default;

        bool canLoad(const std::string& extension) const {
            std::vector<std::string> exts = sourceExtensions();
            return std::find(exts.begin(), exts.end(), extension) != exts.end();
        }

        virtual std::vector<std::string> sourceExtensions() const = 0;

        virtual std::string resourceExtension() const = 0;
    };
}
