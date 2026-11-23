#pragma once

#include <ui/creators/AssetCreator.hpp>
#include <ui/panels/Panel.hpp>
#include <ui/widgets/TextEditField.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

struct ImGuiMultiSelectIO;
struct ImGuiTextBuffer;

namespace TechEngine {
    class EditorCommands;
    struct ListedEntry;

    class AssetBrowserPanel final : public Panel {
    private:
        EditorCommands* m_commands = nullptr;
        std::vector<std::unique_ptr<AssetCreator>> m_creators;
        // Read from the layout before any listing arrives, so it may name a folder that is gone.
        std::string m_openFolder;
        // Indices into the snapshot's listing, valid only while its revision is m_listedRevision.
        std::vector<std::size_t> m_contents;
        std::uint64_t m_listedRevision = 0;
        std::vector<std::string> m_selection;
        std::string m_pendingFolder;
        bool m_createRequested = false;
        // Points into m_creators while the dialog is open.
        AssetCreator* m_activeCreator = nullptr;
        CreateContext m_createContext;
        std::string m_createName;
        TextEditField m_createNameField;

    public:
        explicit AssetBrowserPanel(EditorCommands& commands);

        // Call before the layout loads, or the remembered folder is never read. ImGui keeps this panel's address,
        // so the panel must not move afterwards.
        void registerSettings();

        const std::string& getOpenFolder() const;

        void readSettingsLine(std::string_view line);

        void writeSettings(ImGuiTextBuffer& out) const;

    protected:
        void onDraw(const EditorSnapshot& snapshot) override;

    private:
        void adoptListing(const EditorSnapshot& snapshot);

        void openFolder(const EditorSnapshot& snapshot, std::string folder);

        void drawTree(const EditorSnapshot& snapshot);

        void drawTreeNode(const EditorSnapshot& snapshot, std::string_view folder);

        void drawBreadcrumbs(const EditorSnapshot& snapshot);

        void drawGrid(const EditorSnapshot& snapshot);

        void drawTile(const EditorSnapshot& snapshot, std::size_t item);

        void drawBadge(const ListedEntry& entry, ImVec2 min, ImVec2 max) const;

        void drawTileMenu(const EditorSnapshot& snapshot);

        void drawGridMenu(const EditorSnapshot& snapshot);

        void drawCreateEntries(const CreateContext& context);

        void openCreateDialog(AssetCreator& creator, CreateContext context);

        // The first free output for name, or empty when name is not valid. Only what is free now: the create takes its
        // name when it runs.
        std::string findCreateOutput(const EditorSnapshot& snapshot, std::string_view name) const;

        void drawCreateDialog(const EditorSnapshot& snapshot);

        void applySelectionRequests(const EditorSnapshot& snapshot, ImGuiMultiSelectIO* io);

        bool isSelected(std::string_view path) const;

        static bool isPlaceable(const ListedEntry& entry);

        static std::string_view nameOf(std::string_view path);

        // The last line ends in "..." when the label does not fit.
        static void wrapLabel(std::string_view label, float maxWidth, std::vector<std::string>& out);
    };
}
