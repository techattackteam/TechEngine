#pragma once

#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/assets/ImportBlock.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>

#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace TechEngine {
    class FileAccess;

    struct ImportedFile {
        std::string path;
        StringId typeTag;
        ResourceId id;
        ImportBlock importBlock;
    };

    // Reads only the header, the id and the import block of a model, mesh, material or texture file.
    std::error_code readImportedFile(const FileAccess& files, std::string_view path, ImportedFile& out);

    // Every model, mesh, material and texture file under folder. Any other file is logged and left out.
    std::error_code listImportedFiles(const FileAccess& files, std::string_view folder, std::vector<ImportedFile>& out);
}
