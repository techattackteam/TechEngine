#pragma once
#include <queue>

#include "FileSystem.hpp"
#include "TechEngine/core/fileSystem/IFileWatcher.hpp"

namespace TechEngine {
    class FileWatcher : public IFileWatcher {
        struct WatchedDirectory {
            std::filesystem::path virtualDir;
            std::filesystem::path physicalDir;
            bool recursive = true;
            HANDLE dirHandle = nullptr;
            uint8_t buffer[65536 - sizeof(OVERLAPPED)] = {};
            uint8_t doubleBuffer[65536 - sizeof(OVERLAPPED)] = {};
        };

    public:
        FileWatcher(FileSystem& fileSystem);

        ~FileWatcher();

        void watch(const std::filesystem::path& virtualPath, bool recursive) override;

        void unwatch(const std::filesystem::path& virtualPath) override;

        uint32_t subscribe(std::function<void(const FileChangedEvent&)> callback) override;

        void unsubscribe(uint32_t subscriptionId) override;

        void poll() override;

        void pause() override;

        void resume() override;

    private:
        FileSystem& m_fileSystem;

        std::unordered_map<std::string, WatchedDirectory> m_watchedDirectories;
        mutable std::mutex m_watchesMutex;

        std::unordered_map<uint32_t, std::function<void(const FileChangedEvent&)>> m_callbacks;
        std::atomic<uint32_t> m_nextToken{1};
        std::mutex m_callbackMutex;

        std::queue<FileChangedEvent> m_pendingEvents;
        mutable std::mutex m_eventMutex;

        HANDLE m_iocp = nullptr;
        std::thread m_thread;
        std::atomic<bool> m_running{false};
        std::atomic<bool> m_paused{false};

        static constexpr uintptr_t kShutdownKey = static_cast<uintptr_t>(-1);

        void watcherThread();

        void issueRead(WatchedDirectory& watch);

        void processNotifications(WatchedDirectory& watch, DWORD bytesTransferred);

        static FileChangeAction mapAction(DWORD action);

        std::filesystem::path toVirtualPath(const std::filesystem::path& physicalPath) const;
    };
}
