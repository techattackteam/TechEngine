#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <system_error>
#include <vector>

namespace TechEngine {
    struct DeletePreview {
        std::uint64_t sequence = 0;
        std::vector<std::string> paths;
        std::vector<std::string> users;
        std::size_t heldFileCount = 0;
        std::error_code refusal;
    };
}
