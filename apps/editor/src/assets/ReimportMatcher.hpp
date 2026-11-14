#pragma once

#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/assets/ImportBlock.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace TechEngine {
    class FileAccess;

    inline constexpr std::uint32_t REIMPORT_NO_MATCH = UINT32_MAX;

    struct ImportKey {
        StringId typeTag;
        std::uint32_t sourceIndex = 0;
        std::string sourceName;
    };

    struct ImportedFile {
        std::string path;
        StringId typeTag;
        ResourceId id;
        ImportBlock importBlock;
    };

    class ReimportMatcher {
    public:
        // Fills one entry per wanted key: the index of the old key whose file it keeps, or REIMPORT_NO_MATCH. An old key
        // is kept at most once, and only by a key of its own type. A name matches first, and among old keys of one name
        // the one with the same index wins, then the lowest index. An empty name matches an empty name at the same index.
        static void match(std::span<const ImportKey> oldKeys, std::span<const ImportKey> wanted, std::vector<std::uint32_t>& out);

    private:
        static bool sameTypeAndName(const ImportKey& first, const ImportKey& second);
    };

    // Reads only the header, the id and the import block of a model, mesh, material or texture file.
    std::error_code readImportedFile(const FileAccess& files, std::string_view path, ImportedFile& out);

    // Every model, mesh, material and texture file under folder. Any other file is logged and left out.
    std::error_code listImportedFiles(const FileAccess& files, std::string_view folder, std::vector<ImportedFile>& out);
}
