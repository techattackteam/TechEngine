#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/platform/files/FileError.hpp>
#include <TechEngine/platform/process/Process.hpp>

#include <string>
#include <system_error>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#else
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <cerrno>

extern char** environ;
#endif

namespace TechEngine {
    Process::~Process() {
        release();
    }

    std::error_code Process::start(const std::filesystem::path& executable, const std::filesystem::path& workingDirectory) {
        TE_CHECK(!m_running, "Process::start called while the previous child still runs");

        const std::error_code pathError = checkPaths(executable, workingDirectory);
        if (pathError) {
            return pathError;
        }

        // The child changes directory before it runs the executable, so a relative path would resolve against the new one.
        std::error_code ec;
        const std::filesystem::path absoluteExecutable = std::filesystem::absolute(executable, ec);
        if (ec) {
            TE_LOGGER_WARN("Failed to make {0} absolute: {1}", executable.string(), ec.message());
            return FileError::IoError;
        }

        const std::error_code spawnError = spawn(absoluteExecutable, workingDirectory);
        if (spawnError) {
            return spawnError;
        }
        m_running = true;
        return {};
    }

    bool Process::isRunning() const {
        return m_running;
    }

    std::error_code Process::checkPaths(const std::filesystem::path& executable, const std::filesystem::path& workingDirectory) {
        std::error_code executableError;
        const std::filesystem::file_status executableStatus = std::filesystem::status(executable, executableError);
        if (executableStatus.type() == std::filesystem::file_type::not_found) {
            return FileError::NotFound;
        }
        if (executableError) {
            TE_LOGGER_WARN("Failed to check {0}: {1}", executable.string(), executableError.message());
            return FileError::IoError;
        }
        if (std::filesystem::is_directory(executableStatus)) {
            return FileError::IsADirectory;
        }

        std::error_code directoryError;
        const std::filesystem::file_status directoryStatus = std::filesystem::status(workingDirectory, directoryError);
        if (directoryStatus.type() == std::filesystem::file_type::not_found) {
            return FileError::NotFound;
        }
        if (directoryError) {
            TE_LOGGER_WARN("Failed to check {0}: {1}", workingDirectory.string(), directoryError.message());
            return FileError::IoError;
        }
        if (!std::filesystem::is_directory(directoryStatus)) {
            return FileError::NotADirectory;
        }
        return {};
    }

#if defined(_WIN32)
    std::error_code Process::spawn(const std::filesystem::path& executable, const std::filesystem::path& workingDirectory) {
        // CreateProcessW may write into the command line, so it cannot point into a const string.
        std::wstring commandLine = L"\"" + executable.wstring() + L"\"";
        STARTUPINFOW startupInfo{};
        startupInfo.cb = sizeof(startupInfo);
        PROCESS_INFORMATION processInfo{};
        const BOOL created = CreateProcessW(executable.c_str(), commandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, workingDirectory.c_str(), &startupInfo, &processInfo);
        if (created == FALSE) {
            const DWORD error = GetLastError();
            TE_LOGGER_WARN("Failed to start {0}: {1}", executable.string(), std::system_category().message(static_cast<int>(error)));
            switch (error) {
                case ERROR_FILE_NOT_FOUND:
                case ERROR_PATH_NOT_FOUND:
                    return FileError::NotFound;
                case ERROR_ACCESS_DENIED:
                case ERROR_BAD_EXE_FORMAT:
                    return FileError::AccessDenied;
                default:
                    return FileError::IoError;
            }
        }
        CloseHandle(processInfo.hThread);
        m_handle = processInfo.hProcess;
        return {};
    }

    bool Process::pollExit(int& exitCode) {
        if (!m_running) {
            return false;
        }
        const DWORD waitResult = WaitForSingleObject(m_handle, 0);
        if (waitResult == WAIT_TIMEOUT) {
            return false;
        }

        DWORD code = 0;
        const BOOL known = waitResult == WAIT_OBJECT_0 ? GetExitCodeProcess(m_handle, &code) : FALSE;
        if (known == FALSE) {
            TE_LOGGER_WARN("Failed to read a child process's exit code: {0}", std::system_category().message(static_cast<int>(GetLastError())));
            exitCode = -1;
        } else {
            exitCode = static_cast<int>(code);
        }
        release();
        m_running = false;
        return true;
    }

    void Process::release() {
        if (m_handle != nullptr) {
            CloseHandle(m_handle);
            m_handle = nullptr;
        }
    }
#else
    std::error_code Process::spawn(const std::filesystem::path& executable, const std::filesystem::path& workingDirectory) {
        const std::string path = executable.string();
        const std::string directory = workingDirectory.string();
        posix_spawn_file_actions_t actions;
        posix_spawn_file_actions_init(&actions);
        int result = posix_spawn_file_actions_addchdir_np(&actions, directory.c_str());
        pid_t pid = 0;
        if (result == 0) {
            char* const arguments[] = {const_cast<char*>(path.c_str()), nullptr};
            result = posix_spawn(&pid, path.c_str(), &actions, nullptr, arguments, environ);
        }
        posix_spawn_file_actions_destroy(&actions);

        if (result != 0) {
            TE_LOGGER_WARN("Failed to start {0}: {1}", path, std::system_category().message(result));
            switch (result) {
                case ENOENT:
                    return FileError::NotFound;
                case EACCES:
                case EPERM:
                case ENOEXEC:
                    return FileError::AccessDenied;
                default:
                    return FileError::IoError;
            }
        }
        m_pid = pid;
        return {};
    }

    bool Process::pollExit(int& exitCode) {
        if (!m_running) {
            return false;
        }
        int status = 0;
        const pid_t result = waitpid(m_pid, &status, WNOHANG);
        if (result == 0) {
            return false;
        }
        if (result == -1) {
            const int error = errno;
            if (error == EINTR) {
                return false;
            }
            TE_LOGGER_WARN("Failed to read a child process's exit status: {0}", std::system_category().message(error));
            exitCode = -1;
        } else if (WIFEXITED(status)) {
            exitCode = WEXITSTATUS(status);
        } else {
            exitCode = 128 + WTERMSIG(status);
        }
        release();
        m_running = false;
        return true;
    }

    // An unreaped child stays a zombie until this process exits, which is the cost of never waiting for it.
    void Process::release() {
        m_pid = 0;
    }
#endif
}
