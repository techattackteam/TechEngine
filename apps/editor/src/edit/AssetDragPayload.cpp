#include <edit/AssetDragPayload.hpp>

#include <cstddef>

namespace TechEngine {
    void writeAssetDragPayload(const std::span<const std::string> paths, std::string& out) {
        out.clear();
        for (const std::string& path: paths) {
            out += path;
            out.push_back('\0');
        }
    }

    bool readAssetDragPayload(const std::string_view bytes, std::vector<std::string>& out) {
        out.clear();
        if (bytes.empty() || bytes.back() != '\0') {
            return false;
        }

        std::size_t start = 0;
        while (start < bytes.size()) {
            // The last byte is a NUL, so this always finds one.
            const std::size_t end = bytes.find('\0', start);
            if (end == start) {
                out.clear();
                return false;
            }
            out.emplace_back(bytes.substr(start, end - start));
            start = end + 1;
        }
        return true;
    }
}
