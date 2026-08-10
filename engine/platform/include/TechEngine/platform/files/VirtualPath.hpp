#pragma once

#include <string_view>

namespace TechEngine {
    struct VirtualPathParts {
        std::string_view alias;
        std::string_view relative;
    };

    // out points into virtualPath — both views dangle the moment it does.
    bool splitVirtualPath(std::string_view virtualPath, VirtualPathParts& out);
}
