#pragma once

#include <TechEngine/platform/files/FileError.hpp>

#include <string_view>
#include <system_error>

namespace TechEngine {
    struct VirtualPathParts {
        std::string_view alias;
        std::string_view relative;
    };

    // out points into virtualPath; both views dangle the moment it does.
    std::error_code splitVirtualPath(std::string_view virtualPath, VirtualPathParts& out);
}
