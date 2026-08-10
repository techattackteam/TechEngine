#include <TechEngine/platform/files/MountTable.hpp>
#include <TechEngine/platform/files/VirtualPath.hpp>

#include <algorithm>
#include <utility>

namespace TechEngine {
    // Windows resolves a wrong-case path happily and Linux does not, so exists() alone gives
    // the two CI legs different answers. Every candidate that exists still has to prove its
    // spelling matches the bytes on disk — canonical() reports the real on-disk name.
    static bool matchesOnDiskCase(const MountEntry& entry, std::string_view relative, const std::filesystem::path& candidate) {
        std::error_code ec;

        const std::filesystem::path actual = std::filesystem::canonical(candidate, ec);
        if (ec) {
            return false;
        }

        std::filesystem::path root = entry.canonicalRoot;
        if (root.empty()) {
            root = std::filesystem::canonical(entry.physicalRoot, ec);
            if (ec) {
                return false;
            }
        }

        const std::filesystem::path expected = relative.empty() ? root : root / relative;
        return actual == expected.lexically_normal();
    }

    void MountTable::mount(const std::string& alias, const std::filesystem::path& physicalRoot, int priority) {
        std::error_code ec;
        std::filesystem::path canonicalRoot = std::filesystem::canonical(physicalRoot, ec);
        if (ec) {
            canonicalRoot.clear();
        }

        MountEntry entry{alias, physicalRoot, std::move(canonicalRoot), priority};
        for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
            if (it->priority < priority) {
                m_entries.insert(it, std::move(entry));
                return;
            }
        }
        m_entries.push_back(std::move(entry));
    }

    bool MountTable::unmount(const std::string& alias) {
        const std::size_t removed = std::erase_if(m_entries, [&alias](const MountEntry& entry) {
            return entry.alias == alias;
        });
        return removed > 0;
    }

    void MountTable::clear() {
        m_entries.clear();
    }

    bool MountTable::hasAlias(std::string_view alias) const {
        return std::any_of(m_entries.begin(), m_entries.end(), [alias](const MountEntry& entry) {
            return entry.alias == alias;
        });
    }

    std::size_t MountTable::mountCount() const {
        return m_entries.size();
    }

    std::span<const MountEntry> MountTable::entries() const {
        return m_entries;
    }

    FileResult MountTable::resolveExisting(std::string_view virtualPath, std::filesystem::path& out) const {
        VirtualPathParts parts;
        if (!splitVirtualPath(virtualPath, parts)) {
            return FileResult::InvalidPath;
        }

        bool aliasMounted = false;
        for (const MountEntry& entry: m_entries) {
            if (entry.alias != parts.alias) {
                continue;
            }
            aliasMounted = true;

            const std::filesystem::path candidate = parts.relative.empty() ? entry.physicalRoot : entry.physicalRoot / parts.relative;

            std::error_code ec;
            if (!std::filesystem::exists(candidate, ec)) {
                continue;
            }
            if (!matchesOnDiskCase(entry, parts.relative, candidate)) {
                continue;
            }

            out = candidate;
            return FileResult::Ok;
        }

        return aliasMounted ? FileResult::NotFound : FileResult::NoMount;
    }
}
