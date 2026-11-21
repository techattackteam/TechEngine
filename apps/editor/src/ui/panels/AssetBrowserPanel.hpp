#pragma once

#include <ui/panels/Panel.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

struct ImGuiMultiSelectIO;
struct ImGuiTextBuffer;

namespace TechEngine {
    struct ListedEntry;

    class AssetBrowserPanel final : public Panel {
    private:
        // Read from the layout before any listing arrives, so it may name a folder that is gone.
        std::string m_openFolder;
        // Indices into the snapshot's listing, valid only while its revision is m_listedRevision.
        std::vector<std::size_t> m_contents;
        std::uint64_t m_listedRevision = 0;
        std::vector<std::string> m_selection;
        std::string m_pendingFolder;

    public:
        AssetBrowserPanel();

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

        void applySelectionRequests(const EditorSnapshot& snapshot, ImGuiMultiSelectIO* io);

        bool isSelected(std::string_view path) const;

        static std::string_view nameOf(std::string_view path);

        // The last line ends in "..." when the label does not fit.
        static void wrapLabel(std::string_view label, float maxWidth, std::vector<std::string>& out);
    };
}
