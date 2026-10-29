#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/platform/files/MountTable.hpp>
#include <TechEngine/platform/files/VirtualPath.hpp>

#include <algorithm>
#include <utility>

namespace TechEngine {
    // Windows resolves a wrong-case path happily and Linux does not, so exists() alone gives
    // the two CI legs different answers. Every candidate that exists still has to prove its
    // spelling, one component at a time against its directory's listing. canonical() would
    // also resolve symlinks, so a link below the root would never match the logical path.
    static bool matchesOnDiskCase(const MountEntry& entry, std::string_view relative) {
        std::filesystem::path directory = entry.physicalRoot;
        for (const std::filesystem::path& component: std::filesystem::path(relative)) {
            if (component.empty() || component == ".") {
                continue;
            }

            std::error_code ec;
            std::filesystem::directory_iterator it(directory, ec);
            const std::filesystem::directory_iterator end;
            bool found = false;
            while (!ec && it != end) {
                if (it->path().filename() == component) {
                    found = true;
                    break;
                }
                it.increment(ec);
            }
            if (!found) {
                return false;
            }

            directory /= component;
        }
        return true;
    }

    void MountTable::mount(const std::string& alias, const std::filesystem::path& physicalRoot, int priority) {
        TE_CHECK(!alias.empty(), "A mount alias must not be empty");
        TE_CHECK(alias.find('/') == std::string::npos, "A mount alias must not contain '/': {0}", alias);
        TE_CHECK(alias.find(':') == std::string::npos, "A mount alias must not contain ':'. Mount it without the separator: {0}", alias);

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

    std::error_code MountTable::unmount(const std::string& alias) {
        const std::size_t removed = std::erase_if(m_entries, [&alias](const MountEntry& entry) {
            return entry.alias == alias;
        });
        if (removed == 0) {
            return FileError::NoMount;
        }
        return {};
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

    std::error_code MountTable::resolveExisting(std::string_view virtualPath, std::filesystem::path& out) const {
        VirtualPathParts parts;
        const std::error_code splitError = splitVirtualPath(virtualPath, parts);
        if (splitError) {
            return splitError;
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
            if (!matchesOnDiskCase(entry, parts.relative)) {
                continue;
            }

            out = candidate;
            return {};
        }

        return aliasMounted ? FileError::NotFound : FileError::NoMount;
    }

    std::error_code MountTable::resolveForCreate(std::string_view virtualPath, std::filesystem::path& out) const {
        VirtualPathParts parts;
        const std::error_code splitError = splitVirtualPath(virtualPath, parts);
        if (splitError) {
            return splitError;
        }

        if (parts.relative.empty()) {
            return FileError::InvalidPath;
        }

        for (const MountEntry& entry: m_entries) {
            if (entry.alias != parts.alias) {
                continue;
            }

            out = entry.physicalRoot / parts.relative;
            return {};
        }

        return FileError::NoMount;
    }
}
