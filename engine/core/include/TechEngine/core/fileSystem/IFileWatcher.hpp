#pragma once

#include <functional>
#include <filesystem>

namespace TechEngine {
    enum class FileChangeAction {
        Added,
        Removed,
        Modified,
        Renamed,
    };

    struct FileChangedEvent {
        FileChangeAction action;
        std::filesystem::path physicalPath;
        std::filesystem::path virtualPath;
        std::filesystem::path renamedFromVirtualPath; // Not sure i will keep this
    };

    class IFileWatcher {
    public:
        virtual ~IFileWatcher() = default;

        virtual void watch(const std::filesystem::path& virtualPath, bool recursive) = 0;

        virtual void unwatch(const std::filesystem::path& virtualPath) = 0;

        virtual uint32_t subscribe(std::function<void(const FileChangedEvent&)> callback) = 0;

        virtual void unsubscribe(uint32_t subscriptionId) = 0;

        virtual void poll() = 0;

        virtual void pause() = 0;

        virtual void resume() = 0;
    };
}
