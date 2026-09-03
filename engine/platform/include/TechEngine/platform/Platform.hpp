#pragma once

#include <filesystem>

namespace TechEngine {
    const char* platformVersion();

    const std::filesystem::path& executablePath();
}
