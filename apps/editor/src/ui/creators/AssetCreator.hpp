#pragma once

#include <TechEngine/core/resources/Side.hpp>

#include <assets/index/AssetIndex.hpp>

#include <cstdint>
#include <string>
#include <string_view>

namespace TechEngine {
    class EditorCommands;

    // Copied out of the listing, because the dialog outlives the snapshot it opened from.
    struct CreateContext {
        // Empty when the create starts from the open folder rather than from a selected file.
        std::string sourcePath;
        ListedKind sourceKind = ListedKind::Other;
        // Where the output goes: the source's folder, or the open folder.
        std::string folder;
        Side side = Side::Client;
    };

    // The dialog owns the name, so a creator holds only what its own settings add.
    class AssetCreator {
    public:
        virtual ~AssetCreator() = default;

        // The menu entry, such as "Create material...".
        virtual const char* getLabel() const = 0;

        virtual bool accepts(const CreateContext& context) const = 0;

        virtual std::string getDefaultName(const CreateContext& context) const = 0;

        virtual std::string getOutputPath(const CreateContext& context, std::string_view name, std::uint32_t suffix) const = 0;

        // Called when the dialog opens, before the first drawSettings.
        virtual void begin(const CreateContext&) {
        }

        // The widgets after the name, such as a model's scale. Most creators have none.
        virtual void drawSettings() {
        }

        virtual void submit(EditorCommands& commands, const CreateContext& context, std::string name) = 0;
    };
}
