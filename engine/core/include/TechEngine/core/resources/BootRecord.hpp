#pragma once

#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/Side.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <system_error>
#include <vector>

namespace TechEngine {
    class Reader;
    enum class Role : std::uint8_t;

    inline constexpr StringId BOOT_STARTUP_SCENE_SECTION{"TechEngine.Boot.StartupScene"};
    inline constexpr StringId BOOT_SYSTEM_SETTINGS_SECTION{"TechEngine.Boot.SystemSettings"};
    inline constexpr StringId BOOT_ROLE_PACKS_SECTION{"TechEngine.Boot.RolePacks"};
    inline constexpr StringId BOOT_SIDES_SECTION{"TechEngine.Boot.Sides"};

    inline constexpr std::uint16_t BOOT_SECTION_VERSION = 1;

    struct SystemSettingsEntry {
        std::string system;
        ResourceId settings;
    };

    struct RolePacks {
        Role role{};
        std::vector<std::string> packs;
    };

    struct ResourceSide {
        ResourceId id;
        Side side = Side::Common;
    };

    struct BootRecord {
        ResourceId startupScene;
        std::vector<SystemSettingsEntry> systemSettings;
        std::vector<RolePacks> rolePacks;
        std::vector<ResourceSide> sides;

        void write(std::vector<std::byte>& out) const;

        std::error_code read(std::span<const std::byte> bytes);

    private:
        std::error_code readSection(StringId tag, Reader& reader);
    };
}
