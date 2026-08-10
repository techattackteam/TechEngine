#include <TechEngine/platform/files/VirtualPath.hpp>

#include <cstddef>

namespace TechEngine {
    static bool hasParentSegment(std::string_view path) {
        std::size_t position = 0;

        while ((position = path.find("..", position)) != std::string_view::npos) {
            const bool atStart = position == 0;
            const bool afterSlash = !atStart && path[position - 1] == '/';
            const std::size_t end = position + 2;
            const bool atEnd = end == path.size();
            const bool beforeSlash = !atEnd && path[end] == '/';

            if ((atStart || afterSlash) && (atEnd || beforeSlash)) {
                return true;
            }

            position += 2;
        }

        return false;
    }

    bool splitVirtualPath(std::string_view virtualPath, VirtualPathParts& out) {
        if (virtualPath.find('\\') != std::string_view::npos) {
            return false;
        }

        const std::size_t aliasPosition = virtualPath.find("://");
        if (aliasPosition == std::string_view::npos || aliasPosition == 0) {
            return false;
        }

        const std::string_view alias = virtualPath.substr(0, aliasPosition);
        const std::string_view relative = virtualPath.substr(aliasPosition + 3);

        if (alias.find('/') != std::string_view::npos || alias.find(':') != std::string_view::npos) {
            return false;
        }

        if (!relative.empty() && (relative.front() == '/' || relative.find(':') != std::string_view::npos)) {
            return false;
        }

        if (hasParentSegment(relative)) {
            return false;
        }

        out.alias = alias;
        out.relative = relative;
        return true;
    }
}
