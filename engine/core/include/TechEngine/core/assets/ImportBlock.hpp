#pragma once

#include <cstdint>
#include <string>
#include <system_error>

namespace TechEngine {
    class Reader;
    class Writer;

    enum class UpAxis : std::uint8_t {
        Y,
        Z,
    };

    struct ImportSettings {
        float scale = 1.0f;
        UpAxis upAxis = UpAxis::Y;

        bool operator==(const ImportSettings&) const = default;
    };

    struct ImportBlock {
        std::string sourcePath;
        std::uint32_t sourceIndex = 0;
        std::string sourceName;
        ImportSettings settings;

        void write(Writer& writer) const;

        std::error_code read(Reader& reader);

        bool operator==(const ImportBlock&) const = default;
    };
}
