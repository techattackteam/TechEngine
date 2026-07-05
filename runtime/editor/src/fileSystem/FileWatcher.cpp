#include "FileWatcher.hpp"

#include "core/Logger.hpp"

namespace TechEngine {
    FileWatcher::FileWatcher(FileSystem& fileSystem) : m_fileSystem(fileSystem) {
        m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
        if (!m_iocp || m_iocp == INVALID_HANDLE_VALUE)
            TE_LOGGER_CRITICAL("[FileWatcher] Failed to create IOCP: {}", GetLastError());

        m_running = true;
        m_thread = std::thread(&FileWatcher::watcherThread, this);
    }

    FileWatcher::~FileWatcher() {
        m_running = false;
        if (m_iocp && m_iocp != INVALID_HANDLE_VALUE)
            PostQueuedCompletionStatus(m_iocp, 0,
                                       static_cast<ULONG_PTR>(kShutdownKey), nullptr);
        if (m_thread.joinable()) m_thread.join(); {
            std::scoped_lock lock(m_watchesMutex);
            for (auto& [key, wd]: m_watchedDirectories)
                if (wd.dirHandle && wd.dirHandle != INVALID_HANDLE_VALUE)
                    CloseHandle(wd.dirHandle);
        }
        if (m_iocp && m_iocp != INVALID_HANDLE_VALUE)
            CloseHandle(m_iocp);
    }

    void FileWatcher::watch(const std::filesystem::path& virtualPath, bool recursive) {
        const std::filesystem::path physicalPath = m_fileSystem.resolve(virtualPath);
        if (physicalPath.empty()) {
            TE_LOGGER_WARN("[FileWatcher] Cannot watch '{0}': virtual path does not resolve to a physical path", virtualPath.string());
            return;
        }
        std::scoped_lock lock(m_watchesMutex);
        const std::string key = virtualPath.string();
        if (m_watchedDirectories.count(key) > 0) {
            TE_LOGGER_WARN("[FileWatcher] Already watching '{0}'", virtualPath.string());
            return;
        }
        WatchedDirectory watch;
        watch.virtualDir = virtualPath;
        watch.physicalDir = physicalPath;
        watch.recursive = recursive;

        watch.dirHandle = CreateFileA(
            physicalPath.string().c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
            nullptr
        );

        if (!watch.dirHandle || watch.dirHandle == INVALID_HANDLE_VALUE) {
            TE_LOGGER_ERROR("[FileWatcher] Failed to watch '{0}': {1}", virtualPath.string(), GetLastError());
            m_watchedDirectories.erase(key);
            return;
        }

        auto [it, ok] = m_watchedDirectories.emplace(key, std::move(watch));
        WatchedDirectory& newWatch = it->second;
        if (!CreateIoCompletionPort(newWatch.dirHandle, m_iocp, reinterpret_cast<ULONG_PTR>(&newWatch), 0)) {
            TE_LOGGER_ERROR("[FileWatcher] Failed to create IO completion port for '{0}'", virtualPath.string());
            CloseHandle(newWatch.dirHandle);
            m_watchedDirectories.erase(key);
            return;
        }
        issueRead(newWatch);
        TE_LOGGER_INFO("[FileWatcher] Watching '{0}' (physical: '{1}')", virtualPath.string(), physicalPath.string());
    }

    void FileWatcher::unwatch(const std::filesystem::path& virtualPath) {
        std::scoped_lock lock(m_watchesMutex);
        const std::string key = virtualPath.generic_string();
        if (m_watchedDirectories.count(key) == 0) {
            TE_LOGGER_WARN("[FileWatcher] Cannot unwatch '{0}': not watching", virtualPath.string());
            return;
        }
        WatchedDirectory& watch = m_watchedDirectories[key];
        CloseHandle(watch.dirHandle);
        m_watchedDirectories.erase(key);
        TE_LOGGER_INFO("[FileWatcher] Unwatched '{0}'", virtualPath.string());
    }

    uint32_t FileWatcher::subscribe(std::function<void(const FileChangedEvent&)> callback) {
        const uint32_t token = m_nextToken.fetch_add(1, std::memory_order_relaxed);
        std::scoped_lock lock(m_callbackMutex);
        m_callbacks[token] = callback;
        return token;
    }

    void FileWatcher::unsubscribe(uint32_t subscriptionId) {
        std::scoped_lock lock(m_callbackMutex);
        m_callbacks.erase(subscriptionId);
    }

    void FileWatcher::poll() {
        if (m_paused) {
            return;
        }

        std::queue<FileChangedEvent> local; {
            std::scoped_lock lock(m_eventMutex);
            std::swap(local, m_pendingEvents);
        }
        if (local.empty()) {
            return;
        }

        std::vector<std::function<void(const FileChangedEvent&)>> callbacksCopy; {
            std::scoped_lock lock(m_callbackMutex);
            callbacksCopy.reserve(m_callbacks.size());
            for (auto& [token, cb]: m_callbacks) {
                callbacksCopy.push_back(cb);
            }
        }

        while (!local.empty()) {
            for (auto& cb: callbacksCopy) {
                cb(local.front());
            }
            local.pop();
        }
    }

    void FileWatcher::pause() {
        m_paused = true;
    }

    void FileWatcher::resume() {
        m_paused = false;
    }

    void FileWatcher::watcherThread() {
        while (m_running) {
            DWORD bytes = 0;
            ULONG_PTR key = 0;
            OVERLAPPED* ov = nullptr;

            GetQueuedCompletionStatus(m_iocp, &bytes, &key, &ov, INFINITE);

            if (key == static_cast<ULONG_PTR>(kShutdownKey)) break;
            if (!key) continue;

            auto* wd = reinterpret_cast<WatchedDirectory*>(key);
            if (bytes > 0) {
                processNotifications(*wd, bytes);
            }

            if (m_running) {
                std::scoped_lock lock(m_watchesMutex);
                if (m_watchedDirectories.count(wd->virtualDir.generic_string())) {
                    issueRead(*wd);
                }
            }
        }
    }

    void FileWatcher::issueRead(WatchedDirectory& watch) {
        OVERLAPPED* ov = reinterpret_cast<OVERLAPPED*>(watch.buffer);
        ZeroMemory(ov, sizeof(OVERLAPPED));

        constexpr DWORD filter =
                FILE_NOTIFY_CHANGE_FILE_NAME |
                FILE_NOTIFY_CHANGE_DIR_NAME |
                FILE_NOTIFY_CHANGE_SIZE |
                FILE_NOTIFY_CHANGE_LAST_WRITE |
                FILE_NOTIFY_CHANGE_CREATION;

        const DWORD offset = sizeof(OVERLAPPED);
        ReadDirectoryChangesW(
            watch.dirHandle,
            watch.buffer + offset,
            sizeof(watch.buffer) - offset,
            watch.recursive ? TRUE : FALSE,
            filter,
            nullptr, ov, nullptr);
    }

    void FileWatcher::processNotifications(WatchedDirectory& watch, DWORD bytesTransferred) {
        const DWORD offset = sizeof(OVERLAPPED);
        memcpy(watch.doubleBuffer, watch.buffer + offset, bytesTransferred);

        const auto* info = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(watch.doubleBuffer);

        std::filesystem::path lastRenameFrom;

        while (true) {
            const std::wstring wname(info->FileName, info->FileNameLength / sizeof(wchar_t));
            const std::filesystem::path filename(wname);
            const std::filesystem::path physical = watch.physicalDir / filename;
            const std::filesystem::path virtualPath = toVirtualPath(physical);

            FileChangedEvent event;
            event.physicalPath = physical;
            event.virtualPath = virtualPath;
            event.action = mapAction(info->Action);
            if (info->Action == FILE_ACTION_RENAMED_OLD_NAME) {
                lastRenameFrom = virtualPath;
            } else if (info->Action == FILE_ACTION_RENAMED_NEW_NAME) {
                event.renamedFromVirtualPath = lastRenameFrom;
                lastRenameFrom.clear();
            } //
            {
                std::scoped_lock lock(m_eventMutex);
                m_pendingEvents.push(event);
            }

            if (info->NextEntryOffset == 0) {
                break;
            }
            info = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(
                reinterpret_cast<const uint8_t*>(info) + info->NextEntryOffset);
        }
    }

    FileChangeAction FileWatcher::mapAction(DWORD action) {
        switch (action) {
            case FILE_ACTION_ADDED: return FileChangeAction::Added;
            case FILE_ACTION_REMOVED: return FileChangeAction::Removed;
            case FILE_ACTION_MODIFIED: return FileChangeAction::Modified;
            case FILE_ACTION_RENAMED_OLD_NAME:
            case FILE_ACTION_RENAMED_NEW_NAME: return FileChangeAction::Renamed;
            default: return FileChangeAction::Modified;
        }
    }

    std::filesystem::path FileWatcher::toVirtualPath(const std::filesystem::path& physicalPath) const {
        std::scoped_lock lock(m_watchesMutex);
        std::filesystem::path best;
        size_t bestLen = 0;

        for (const auto& [key, wd]: m_watchedDirectories) {
            const std::string physStr = wd.physicalDir.string();
            const std::string pathStr = physicalPath.string();
            if (pathStr.size() > physStr.size() &&
                pathStr.compare(0, physStr.size(), physStr) == 0 &&
                physStr.size() > bestLen) {
                best = wd.virtualDir;
                bestLen = physStr.size();
            }
        }

        if (best.empty()) return physicalPath;

        const std::filesystem::path& physRoot = m_watchedDirectories.at(best.generic_string()).physicalDir;
        return best / std::filesystem::relative(physicalPath, physRoot);
    }
}
