#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/platform/Platform.hpp>

#include <filesystem>
#include <system_error>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

namespace TechEngine {
    static std::filesystem::path resolveExecutablePath() {
#if defined(_WIN32)
        wchar_t buffer[MAX_PATH];
        const unsigned long length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        TE_CHECK(length != 0, "GetModuleFileNameW failed");
        TE_CHECK(length < MAX_PATH, "GetModuleFileNameW buffer too small");
        return std::filesystem::path(buffer);
#else
        std::error_code ec;
        std::filesystem::path path = std::filesystem::read_symlink("/proc/self/exe", ec);
        TE_CHECK(!ec, "read_symlink(/proc/self/exe) failed");
        return path;
#endif
    }

    const std::filesystem::path& executablePath() {
        static const std::filesystem::path path = resolveExecutablePath(); // Caches the executable path on first call
        return path;
    }
}
