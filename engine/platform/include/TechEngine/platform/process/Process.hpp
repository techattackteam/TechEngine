#pragma once

#include <filesystem>
#include <system_error>

namespace TechEngine {
    class Process {
    private:
#if defined(_WIN32)
        void* m_handle = nullptr;
#else
        int m_pid = 0;
#endif
        bool m_running = false;

    public:
        Process() = default;

        // Neither waits for a running child nor stops it.
        ~Process();

        Process(const Process&) = delete;

        Process& operator=(const Process&) = delete;

        Process(Process&&) = delete;

        Process& operator=(Process&&) = delete;

        // The child inherits this process's console and standard streams.
        std::error_code start(const std::filesystem::path& executable, const std::filesystem::path& workingDirectory);

        // Stays true after the child exits, until pollExit() sees the exit.
        bool isRunning() const;

        // True exactly once per start(). A child a signal killed on Linux reports 128 plus the signal number.
        bool pollExit(int& exitCode);

    private:
        static std::error_code checkPaths(const std::filesystem::path& executable, const std::filesystem::path& workingDirectory);

        std::error_code spawn(const std::filesystem::path& executable, const std::filesystem::path& workingDirectory);

        void release();
    };
}
