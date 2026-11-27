#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/model/Model.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/core/texture/Texture.hpp>

#include <assets/index/AssetIndex.hpp>
#include <assets/index/AssetNaming.hpp>
#include <edit/EditorCommands.hpp>
#include <edit/snapshot/EditorSnapshot.hpp>
#include <ui/creators/AssetCreators.hpp>
#include <ui/panels/AssetBrowserPanel.hpp>
#include <ui/widgets/AssetDragDrop.hpp>

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace TechEngine {
    static constexpr const char* SETTINGS_TYPE_NAME = "AssetBrowser";
    static constexpr std::string_view OPEN_FOLDER_KEY = "OpenFolder=";
    static constexpr float TREE_WIDTH = 250.0f;
    static constexpr float BREADCRUMB_ROUNDING = 4.0f;
    static constexpr float TILE_ICON_SIZE = 64.0f;
    static constexpr float TILE_PADDING = 16.0f;
    static constexpr float TILE_COLUMN_WIDTH = TILE_ICON_SIZE + TILE_PADDING * 2.0f;
    static constexpr float TILE_LABEL_WIDTH = TILE_COLUMN_WIDTH - 4.0f;
    static constexpr float TILE_ROUNDING = 6.0f;
    // Inside the cell, because the table clips anything drawn above its first row.
    static constexpr float TILE_TOP_MARGIN = TILE_PADDING * 0.25f;
    static constexpr std::size_t TILE_LABEL_LINES = 3;
    static constexpr std::string_view TRUNCATION_MARK = "...";
    static constexpr float BADGE_ROUNDING = 4.0f;
    static constexpr ImU32 FOLDER_COLOR = IM_COL32(220, 180, 90, 255);
    static constexpr ImU32 FOLDER_TAB_COLOR = IM_COL32(185, 145, 65, 255);
    static constexpr ImU32 SCENE_BADGE_COLOR = IM_COL32(150, 110, 200, 255);
    static constexpr ImU32 MODEL_BADGE_COLOR = IM_COL32(80, 140, 210, 255);
    static constexpr ImU32 MESH_BADGE_COLOR = IM_COL32(70, 165, 155, 255);
    static constexpr ImU32 MATERIAL_BADGE_COLOR = IM_COL32(210, 130, 70, 255);
    static constexpr ImU32 TEXTURE_BADGE_COLOR = IM_COL32(110, 170, 80, 255);
    static constexpr ImU32 UNKNOWN_ASSET_BADGE_COLOR = IM_COL32(120, 120, 120, 255);
    static constexpr ImU32 MODEL_SOURCE_BADGE_COLOR = IM_COL32(115, 125, 150, 255);
    static constexpr ImU32 SKY_SOURCE_BADGE_COLOR = IM_COL32(200, 100, 140, 255);
    static constexpr ImU32 OTHER_BADGE_COLOR = IM_COL32(80, 80, 80, 255);
    static constexpr ImU32 BADGE_LABEL_COLOR = IM_COL32(255, 255, 255, 235);
    static constexpr const char* CREATE_POPUP_NAME = "Create";
    static constexpr const char* DELETE_POPUP_NAME = "Delete";
    static constexpr std::size_t DELETE_USER_ROWS = 10;
    static constexpr float DELETE_LIST_WIDTH_EMS = 32.0f;
    static constexpr ImVec4 DELETE_REFUSAL_COLOR = ImVec4(0.95f, 0.80f, 0.30f, 1.00f);
    static constexpr float CREATE_NAME_WIDTH_EMS = 16.0f;

    static void* openBrowserSettings(ImGuiContext*, ImGuiSettingsHandler* handler, const char*) {
        return handler->UserData;
    }

    static void readBrowserSettingsLine(ImGuiContext*, ImGuiSettingsHandler* handler, void*, const char* line) {
        static_cast<AssetBrowserPanel*>(handler->UserData)->readSettingsLine(line);
    }

    static void writeBrowserSettings(ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* out) {
        static_cast<const AssetBrowserPanel*>(handler->UserData)->writeSettings(*out);
    }

    AssetBrowserPanel::AssetBrowserPanel(EditorCommands& commands) : Panel("Asset Browser"), m_commands(&commands) {
        // A new kind of asset becomes creatable from the browser by adding its creator here, in menu order.
        m_creators.push_back(std::make_unique<ModelCreator>(ImportMode::Model));
        m_creators.push_back(std::make_unique<ModelCreator>(ImportMode::MergedMesh));
        m_creators.push_back(std::make_unique<SkyCreator>());
        m_creators.push_back(std::make_unique<FolderCreator>());
        m_creators.push_back(std::make_unique<MaterialCreator>());
    }

    void AssetBrowserPanel::registerSettings() {
        ImGuiSettingsHandler handler;
        handler.TypeName = SETTINGS_TYPE_NAME;
        // AddSettingsHandler does not hash the name, and the loader finds a handler by this hash alone.
        handler.TypeHash = ImHashStr(SETTINGS_TYPE_NAME);
        handler.ReadOpenFn = &openBrowserSettings;
        handler.ReadLineFn = &readBrowserSettingsLine;
        handler.WriteAllFn = &writeBrowserSettings;
        handler.UserData = this;
        ImGui::AddSettingsHandler(&handler);
    }

    const std::string& AssetBrowserPanel::getOpenFolder() const {
        return m_openFolder;
    }

    void AssetBrowserPanel::readSettingsLine(const std::string_view line) {
        if (line.starts_with(OPEN_FOLDER_KEY)) {
            m_openFolder = line.substr(OPEN_FOLDER_KEY.size());
        }
    }

    void AssetBrowserPanel::writeSettings(ImGuiTextBuffer& out) const {
        out.appendf("[%s][Panel]\n", SETTINGS_TYPE_NAME);
        out.appendf("%.*s%s\n", static_cast<int>(OPEN_FOLDER_KEY.size()), OPEN_FOLDER_KEY.data(), m_openFolder.c_str());
    }

    void AssetBrowserPanel::onDraw(const EditorSnapshot& snapshot) {
        adoptListing(snapshot);
        if (ImGui::BeginTable("##browser", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Folders", ImGuiTableColumnFlags_None, TREE_WIDTH);
            ImGui::TableSetupColumn("Files", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::BeginChild("##folders", ImVec2(0.0f, ImGui::GetContentRegionAvail().y));
            drawTree(snapshot);
            ImGui::EndChild();

            ImGui::TableSetColumnIndex(1);
            drawBreadcrumbs(snapshot);
            // Multi-select scopes its box to the current window, so the grid needs a window of its own.
            ImGui::BeginChild("##files", ImVec2(0.0f, ImGui::GetContentRegionAvail().y));
            drawGrid(snapshot);
            ImGui::EndChild();

            ImGui::EndTable();
        }
        // The tile's menu only asks for a dialog: OpenPopup and BeginPopupModal must resolve the name against the same
        // ID stack, and the tile pushes its own ID.
        drawCreateDialog(snapshot);
        drawDeleteDialog(snapshot);
    }

    void AssetBrowserPanel::adoptListing(const EditorSnapshot& snapshot) {
        // Revision 0 means no listing has arrived yet, so a remembered folder waits for the first one.
        if (snapshot.listingRevision == 0 || snapshot.listingRevision == m_listedRevision) {
            return;
        }
        m_listedRevision = snapshot.listingRevision;

        std::string nearest;
        const bool underSide = snapshot.findNearestFolder(m_openFolder, nearest);
        if (!underSide) {
            for (const SideFolder& side: snapshot.sideFolders) {
                if (side.side == Side::Client) {
                    nearest = side.path;
                }
            }
        }
        if (nearest != m_openFolder) {
            openFolder(snapshot, std::move(nearest));
            return;
        }

        std::erase_if(m_selection, [&snapshot](const std::string& path) {
            return std::ranges::find(snapshot.listing, path, &ListedEntry::path) == snapshot.listing.end();
        });
        snapshot.collectFolderContents(m_openFolder, m_contents);
    }

    void AssetBrowserPanel::openFolder(const EditorSnapshot& snapshot, std::string folder) {
        if (folder == m_openFolder) {
            return;
        }
        m_openFolder = std::move(folder);
        m_selection.clear();
        snapshot.collectFolderContents(m_openFolder, m_contents);
        ImGui::MarkIniSettingsDirty();
    }

    void AssetBrowserPanel::drawTree(const EditorSnapshot& snapshot) {
        for (const SideFolder& side: snapshot.sideFolders) {
            ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
            drawTreeNode(snapshot, side.path);
        }
    }

    void AssetBrowserPanel::drawTreeNode(const EditorSnapshot& snapshot, const std::string_view folder) {
        std::vector<std::size_t> contents;
        snapshot.collectFolderContents(folder, contents);
        const bool hasSubfolders = !contents.empty() && snapshot.listing[contents.front()].kind == ListedKind::Folder;

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth;
        if (folder == m_openFolder) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        if (!hasSubfolders) {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }

        ImGui::PushID(folder.data(), folder.data() + folder.size());
        const std::string_view label = nameOf(folder);
        const bool open = ImGui::TreeNodeEx("##folder", flags, "%.*s", static_cast<int>(label.size()), label.data());
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            openFolder(snapshot, std::string(folder));
        }
        if (open) {
            for (const std::size_t index: contents) {
                const ListedEntry& entry = snapshot.listing[index];
                if (entry.kind != ListedKind::Folder) {
                    break;
                }
                drawTreeNode(snapshot, entry.path);
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    void AssetBrowserPanel::drawBreadcrumbs(const EditorSnapshot& snapshot) {
        std::vector<std::string> crumbs;
        snapshot.collectBreadcrumbs(m_openFolder, crumbs);

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, BREADCRUMB_ROUNDING);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
        std::string clicked;
        for (std::size_t i = 0; i < crumbs.size(); i++) {
            const std::string name(nameOf(crumbs[i]));
            if (i + 1 == crumbs.size()) {
                ImGui::TextUnformatted(name.c_str());
                break;
            }
            ImGui::PushID(static_cast<int>(i));
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            if (ImGui::SmallButton(name.c_str())) {
                clicked = crumbs[i];
            }
            ImGui::PopStyleColor();
            ImGui::PopID();
            ImGui::SameLine();
            ImGui::TextDisabled("/");
            ImGui::SameLine();
        }
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar();
        ImGui::Separator();

        if (!clicked.empty()) {
            openFolder(snapshot, std::move(clicked));
        }
    }

    void AssetBrowserPanel::drawGrid(const EditorSnapshot& snapshot) {
        const int columnCount = std::max(1, static_cast<int>(ImGui::GetContentRegionAvail().x / TILE_COLUMN_WIDTH));
        const ImGuiMultiSelectFlags selectFlags = ImGuiMultiSelectFlags_BoxSelect2d | ImGuiMultiSelectFlags_ClearOnClickVoid;
        ImGuiMultiSelectIO* selectionIO = ImGui::BeginMultiSelect(selectFlags, static_cast<int>(m_selection.size()), static_cast<int>(m_contents.size()));
        applySelectionRequests(snapshot, selectionIO);
        if (ImGui::BeginTable("##tiles", columnCount, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoSavedSettings)) {
            for (std::size_t item = 0; item < m_contents.size(); item++) {
                ImGui::TableNextColumn();
                drawTile(snapshot, item);
            }
            ImGui::EndTable();
        }
        // Before EndMultiSelect, which counts a popup as part of the grid: drawn after it, a click on an entry reaches
        // EndMultiSelect while nothing is hovered yet, so it starts a box select in the void and the entry never sees it.
        if (ImGui::BeginPopupContextWindow("##gridMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
            drawGridMenu(snapshot);
            ImGui::EndPopup();
        }
        ImGuiMultiSelectIO* endIO = ImGui::EndMultiSelect();
        const bool clearedOnVoid = isOnlyClear(endIO);
        applySelectionRequests(snapshot, endIO);
        // A click on empty grid space clears the selection, but ImGui keeps its nav cursor on the last tile clicked, and
        // multi-select always draws that cursor, so the tile would still look selected.
        if (clearedOnVoid && ImGui::GetCurrentContext()->NavWindow != nullptr) {
            ImGui::SetNavID(0, ImGuiNavLayer_Main, ImGui::GetCurrentContext()->NavFocusScopeId, ImRect());
        }

        // Opened only now, because opening rebuilds m_contents and clears the selection the requests refer to.
        if (!m_pendingFolder.empty()) {
            openFolder(snapshot, std::exchange(m_pendingFolder, {}));
        }
    }

    void AssetBrowserPanel::drawTile(const EditorSnapshot& snapshot, const std::size_t item) {
        const ListedEntry& entry = snapshot.listing[m_contents[item]];
        const std::string_view name = nameOf(entry.path);
        const float lineHeight = ImGui::GetTextLineHeightWithSpacing();
        const float labelTop = TILE_TOP_MARGIN + TILE_ICON_SIZE + ImGui::GetStyle().ItemSpacing.y;
        const float cellWidth = ImGui::GetContentRegionAvail().x;
        const float cellHeight = labelTop + static_cast<float>(TILE_LABEL_LINES) * lineHeight;
        const ImVec2 cellStart = ImGui::GetCursorScreenPos();
        const bool selected = isSelected(entry.path);
        std::vector<std::string> lines;
        wrapLabel(name, TILE_LABEL_WIDTH, lines);

        ImGui::PushID(entry.path.data(), entry.path.data() + entry.path.size());
        // HeaderHovered and HeaderActive are grays, so a hovered or pressed selected tile would otherwise lose its
        // selection color.
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyle().Colors[ImGuiCol_Header]);
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGui::GetStyle().Colors[ImGuiCol_Header]);
        }
        // The Selectable draws the highlight and then ImGui's nav outline over it, both on its rectangle and with this
        // rounding. A highlight drawn by the tile afterwards would cover most of the outline.
        ImGui::PushStyleVar(ImGuiStyleVar_SelectableRounding, TILE_ROUNDING);
        ImGui::SetNextItemSelectionUserData(static_cast<ImGuiSelectionUserData>(item));
        // Padded by half the item spacing, the Selectable's rectangle is wider than the tile, and the table clips the pad
        // on the outer side of the first and last column.
        ImGui::Selectable("##tile", selected, ImGuiSelectableFlags_NoPadWithHalfSpacing, ImVec2(cellWidth, cellHeight));
        ImGui::PopStyleVar();
        if (selected) {
            ImGui::PopStyleColor(2);
        }
        const bool hovered = ImGui::IsItemHovered();
        if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            if (entry.kind == ListedKind::Folder) {
                m_pendingFolder = entry.path;
            } else if (EditorSnapshot::isPlaceable(entry)) {
                m_commands->placeModel(entry.path, Entity{});
            }
        }
        if (ImGui::BeginDragDropSource()) {
            beginTileDrag(snapshot, entry, selected);
            ImGui::EndDragDropSource();
        }
        if (hovered && !lines.empty() && lines.back().ends_with(TRUNCATION_MARK)) {
            ImGui::SetTooltip("%.*s", static_cast<int>(name.size()), name.data());
        }
        if (ImGui::BeginPopupContextItem("##tileMenu")) {
            drawTileMenu(snapshot);
            ImGui::EndPopup();
        }
        ImGui::PopID();

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        const ImVec2 iconMin(cellStart.x + (cellWidth - TILE_ICON_SIZE) * 0.5f, cellStart.y + TILE_TOP_MARGIN);
        drawBadge(entry, iconMin, ImVec2(iconMin.x + TILE_ICON_SIZE, iconMin.y + TILE_ICON_SIZE));

        const ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);
        float lineTop = cellStart.y + labelTop;
        for (const std::string& line: lines) {
            const float lineWidth = ImGui::CalcTextSize(line.c_str()).x;
            drawList->AddText(ImVec2(cellStart.x + (cellWidth - lineWidth) * 0.5f, lineTop), textColor, line.c_str());
            lineTop += lineHeight;
        }
    }

    void AssetBrowserPanel::drawBadge(const ListedEntry& entry, const ImVec2 min, const ImVec2 max) const {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const float width = max.x - min.x;
        const float height = max.y - min.y;

        if (entry.kind == ListedKind::Folder) {
            const float left = min.x + width * 0.05f;
            const float right = max.x - width * 0.05f;
            const float bodyTop = min.y + height * 0.25f;
            drawList->AddRectFilled(ImVec2(left, min.y + height * 0.12f), ImVec2(min.x + width * 0.45f, bodyTop + BADGE_ROUNDING), FOLDER_TAB_COLOR, BADGE_ROUNDING);
            drawList->AddRectFilled(ImVec2(left, bodyTop), ImVec2(right, max.y - height * 0.1f), FOLDER_COLOR, BADGE_ROUNDING);
            return;
        }

        const char* label = "FILE";
        ImU32 color = OTHER_BADGE_COLOR;
        if (entry.kind == ListedKind::ModelSource) {
            label = "SRC";
            color = MODEL_SOURCE_BADGE_COLOR;
        } else if (entry.kind == ListedKind::SkySource) {
            label = "HDR";
            color = SKY_SOURCE_BADGE_COLOR;
        } else if (entry.kind == ListedKind::Asset) {
            label = "ASSET";
            color = UNKNOWN_ASSET_BADGE_COLOR;
            if (entry.typeTag == SCENE_TYPE_TAG) {
                label = "SCENE";
                color = SCENE_BADGE_COLOR;
            } else if (entry.typeTag == MODEL_TYPE_TAG) {
                label = "MODEL";
                color = MODEL_BADGE_COLOR;
            } else if (entry.typeTag == MESH_TYPE_TAG) {
                label = "MESH";
                color = MESH_BADGE_COLOR;
            } else if (entry.typeTag == MATERIAL_TYPE_TAG) {
                label = "MAT";
                color = MATERIAL_BADGE_COLOR;
            } else if (entry.typeTag == TEXTURE_TYPE_TAG) {
                label = "TEX";
                color = TEXTURE_BADGE_COLOR;
            }
        }

        const ImVec2 inset(width * 0.1f, height * 0.1f);
        drawList->AddRectFilled(ImVec2(min.x + inset.x, min.y + inset.y), ImVec2(max.x - inset.x, max.y - inset.y), color, BADGE_ROUNDING);
        const ImVec2 labelSize = ImGui::CalcTextSize(label);
        drawList->AddText(ImVec2(min.x + (width - labelSize.x) * 0.5f, min.y + (height - labelSize.y) * 0.5f), BADGE_LABEL_COLOR, label);
    }

    void AssetBrowserPanel::drawTileMenu(const EditorSnapshot& snapshot) {
        std::vector<const ListedEntry*> selected;
        for (const std::string& path: m_selection) {
            const auto found = std::ranges::find(snapshot.listing, path, &ListedEntry::path);
            if (found != snapshot.listing.end()) {
                selected.push_back(&*found);
            }
        }

        if (selected.size() == 1) {
            const ListedEntry& entry = *selected.front();
            CreateContext context;
            context.sourcePath = entry.path;
            context.sourceKind = entry.kind;
            context.folder = std::string(AssetNaming::folderOf(entry.path));
            context.side = entry.side;
            drawCreateEntries(context);
        }

        if (std::ranges::any_of(
                selected,
                [](const ListedEntry* entry) {
                    return EditorSnapshot::isPlaceable(*entry);
                }) &&
            ImGui::MenuItem("Place in scene")) {
            for (const ListedEntry* entry: selected) {
                if (EditorSnapshot::isPlaceable(*entry)) {
                    m_commands->placeModel(entry->path, Entity{});
                }
            }
        }

        if (ImGui::MenuItem("Delete...", nullptr, false, !selected.empty())) {
            std::vector<std::string> paths;
            for (const ListedEntry* entry: selected) {
                paths.push_back(entry->path);
            }
            openDeleteDialog(std::move(paths));
        }
    }

    void AssetBrowserPanel::drawGridMenu(const EditorSnapshot& snapshot) {
        SideFolder side;
        const bool underSide = snapshot.findSideFolder(m_openFolder, side);
        if (underSide) {
            CreateContext context;
            context.folder = m_openFolder;
            context.side = side.side;
            drawCreateEntries(context);
            ImGui::Separator();
        }
        if (ImGui::MenuItem("Refresh")) {
            m_commands->refreshAssets();
        }
    }

    void AssetBrowserPanel::drawCreateEntries(const CreateContext& context) {
        for (const std::unique_ptr<AssetCreator>& creator: m_creators) {
            if (creator->accepts(context) && ImGui::MenuItem(creator->getLabel())) {
                openCreateDialog(*creator, context);
            }
        }
    }

    void AssetBrowserPanel::openCreateDialog(AssetCreator& creator, CreateContext context) {
        m_activeCreator = &creator;
        m_createContext = std::move(context);
        m_createName = creator.getDefaultName(m_createContext);
        creator.begin(m_createContext);
        m_createRequested = true;
    }

    std::string AssetBrowserPanel::findCreateOutput(const EditorSnapshot& snapshot, const std::string_view name) const {
        if (m_activeCreator == nullptr || !AssetNaming::isValidName(name)) {
            return {};
        }
        return snapshot.firstFreePath([this, name](const std::uint32_t suffix) {
            return m_activeCreator->getOutputPath(m_createContext, name, suffix);
        });
    }

    void AssetBrowserPanel::drawCreateDialog(const EditorSnapshot& snapshot) {
        if (m_createRequested) {
            ImGui::OpenPopup(CREATE_POPUP_NAME);
            m_createRequested = false;
        }

        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (!ImGui::BeginPopupModal(CREATE_POPUP_NAME, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            return;
        }
        if (m_activeCreator == nullptr) {
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return;
        }

        ImGui::TextDisabled("%s", m_createContext.sourcePath.empty() ? "In" : "From");
        ImGui::SameLine();
        ImGui::TextUnformatted(m_createContext.sourcePath.empty() ? m_createContext.folder.c_str() : m_createContext.sourcePath.c_str());

        if (ImGui::IsWindowAppearing()) {
            m_createNameField.requestFocus();
        }
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * CREATE_NAME_WIDTH_EMS);
        std::string name;
        // The name is taken when its edit ends, which a click on Create does before the click lands.
        const bool renamed = m_createNameField.draw("Name", m_createName, name);
        if (renamed) {
            m_createName = std::move(name);
        }
        // Worked out every frame from what the field shows, so it follows each key and a refresh behind the dialog.
        const std::string& shownName = m_createNameField.isEditing() ? m_createNameField.getText() : m_createName;
        const bool validName = AssetNaming::isValidName(shownName);
        if (validName) {
            const std::string output = findCreateOutput(snapshot, shownName);
            ImGui::TextDisabled("Writes");
            ImGui::SameLine();
            ImGui::TextUnformatted(output.c_str());
        } else {
            ImGui::TextDisabled("A name cannot be empty, end in a dot or a space, or hold / \\ : * ? \" < > |");
        }

        ImGui::Spacing();
        m_activeCreator->drawSettings();
        ImGui::Spacing();

        ImGui::BeginDisabled(!validName);
        const bool createClicked = ImGui::Button("Create");
        ImGui::EndDisabled();
        ImGui::SameLine();
        const bool cancelled = ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape, false);

        if (createClicked) {
            m_activeCreator->submit(*m_commands, m_createContext, m_createName);
            m_activeCreator = nullptr;
            ImGui::CloseCurrentPopup();
        } else if (cancelled) {
            m_activeCreator = nullptr;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    void AssetBrowserPanel::openDeleteDialog(std::vector<std::string> paths) {
        m_deleteSequence++;
        m_deletePaths = std::move(paths);
        m_commands->previewDelete(m_deletePaths, m_deleteSequence);
        m_deleteRequested = true;
    }

    void AssetBrowserPanel::drawDeleteDialog(const EditorSnapshot& snapshot) {
        if (m_deleteRequested) {
            ImGui::OpenPopup(DELETE_POPUP_NAME);
            m_deleteRequested = false;
        }

        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (!ImGui::BeginPopupModal(DELETE_POPUP_NAME, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            return;
        }

        ImGui::TextUnformatted(m_deletePaths.size() == 1 ? "Delete this file or folder?" : "Delete these files and folders?");
        for (const std::string& path: m_deletePaths) {
            ImGui::BulletText("%s", path.c_str());
        }
        ImGui::Spacing();

        const DeletePreview& preview = snapshot.deletePreview;
        const bool answered = preview.sequence == m_deleteSequence;
        if (!answered) {
            ImGui::TextDisabled("Finding the files that use it...");
        } else {
            if (preview.heldFileCount > 0) {
                ImGui::Text("The folders hold %zu files, which are deleted with them.", preview.heldFileCount);
            }
            if (preview.users.empty()) {
                ImGui::TextDisabled("No other file uses it.");
            } else {
                if (preview.users.size() == 1) {
                    ImGui::TextUnformatted("1 file uses it, and will load it as missing:");
                } else {
                    ImGui::Text("%zu files use it, and will load it as missing:", preview.users.size());
                }
                const float rows = static_cast<float>(std::min<std::size_t>(preview.users.size(), DELETE_USER_ROWS));
                const ImVec2 listSize(ImGui::GetFontSize() * DELETE_LIST_WIDTH_EMS, ImGui::GetTextLineHeightWithSpacing() * rows + ImGui::GetStyle().WindowPadding.y * 2.0f);
                ImGui::BeginChild("##deleteUsers", listSize, ImGuiChildFlags_Borders);
                for (const std::string& user: preview.users) {
                    ImGui::TextUnformatted(user.c_str());
                }
                ImGui::EndChild();
            }
            if (preview.refusal) {
                ImGui::TextColored(DELETE_REFUSAL_COLOR, "Cannot delete: %s", preview.refusal.message().c_str());
            } else {
                ImGui::TextUnformatted("This cannot be undone.");
            }
        }
        ImGui::Spacing();

        ImGui::BeginDisabled(!answered || static_cast<bool>(preview.refusal));
        const bool deleteClicked = ImGui::Button("Delete");
        ImGui::EndDisabled();
        ImGui::SameLine();
        const bool cancelled = ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape, false);

        if (deleteClicked) {
            m_commands->deleteAssets(std::move(m_deletePaths));
            m_deletePaths.clear();
            ImGui::CloseCurrentPopup();
        } else if (cancelled) {
            m_deletePaths.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    void AssetBrowserPanel::applySelectionRequests(const EditorSnapshot& snapshot, ImGuiMultiSelectIO* io) {
        const ImGuiSelectionUserData itemCount = static_cast<ImGuiSelectionUserData>(m_contents.size());
        for (const ImGuiSelectionRequest& request: io->Requests) {
            if (request.Type == ImGuiSelectionRequestType_SetAll) {
                m_selection.clear();
                if (request.Selected) {
                    for (const std::size_t index: m_contents) {
                        m_selection.push_back(snapshot.listing[index].path);
                    }
                }
                continue;
            }
            if (request.Type != ImGuiSelectionRequestType_SetRange) {
                continue;
            }

            for (ImGuiSelectionUserData item = request.RangeFirstItem; item <= request.RangeLastItem && item < itemCount; item++) {
                const std::string& path = snapshot.listing[m_contents[static_cast<std::size_t>(item)]].path;
                const auto found = std::ranges::find(m_selection, path);
                if (request.Selected && found == m_selection.end()) {
                    m_selection.push_back(path);
                } else if (!request.Selected && found != m_selection.end()) {
                    m_selection.erase(found);
                }
            }
        }
    }

    bool AssetBrowserPanel::isOnlyClear(const ImGuiMultiSelectIO* io) {
        if (io->Requests.Size != 1) {
            return false;
        }
        const ImGuiSelectionRequest& request = io->Requests[0];
        return request.Type == ImGuiSelectionRequestType_SetAll && !request.Selected;
    }

    bool AssetBrowserPanel::isSelected(const std::string_view path) const {
        return std::ranges::find(m_selection, path) != m_selection.end();
    }

    void AssetBrowserPanel::beginTileDrag(const EditorSnapshot& snapshot, const ListedEntry& entry, const bool selected) {
        std::vector<std::string> dragged;
        if (selected) {
            // m_selection is in click order; the grid's order is the one the drop sees.
            for (const std::size_t index: m_contents) {
                const std::string& path = snapshot.listing[index].path;
                if (isSelected(path)) {
                    dragged.push_back(path);
                }
            }
        } else {
            dragged.push_back(entry.path);
        }
        setAssetDragPayload(dragged);

        if (dragged.size() == 1) {
            const std::string_view name = nameOf(dragged.front());
            ImGui::TextUnformatted(name.data(), name.data() + name.size());
        } else {
            ImGui::Text("%zu items", dragged.size());
        }
    }

    std::string_view AssetBrowserPanel::nameOf(const std::string_view path) {
        const std::size_t slash = path.rfind('/');
        if (slash == std::string_view::npos) {
            return path;
        }
        return path.substr(slash + 1);
    }

    void AssetBrowserPanel::wrapLabel(const std::string_view label, const float maxWidth, std::vector<std::string>& out) {
        out.clear();
        const auto widthOf = [](const std::string_view text) {
            return ImGui::CalcTextSize(text.data(), text.data() + text.size()).x;
        };

        std::string_view remaining = label;
        while (!remaining.empty() && out.size() < TILE_LABEL_LINES) {
            std::size_t fitLength = remaining.size();
            while (fitLength > 0 && widthOf(remaining.substr(0, fitLength)) > maxWidth) {
                fitLength--;
            }
            if (fitLength == remaining.size()) {
                out.emplace_back(remaining);
                return;
            }

            if (out.size() + 1 == TILE_LABEL_LINES) {
                std::string truncated(remaining.substr(0, fitLength));
                while (!truncated.empty() && widthOf(truncated + std::string(TRUNCATION_MARK)) > maxWidth) {
                    truncated.pop_back();
                }
                out.push_back(truncated + std::string(TRUNCATION_MARK));
                return;
            }

            std::size_t breakPosition = fitLength;
            while (breakPosition > 0 && remaining[breakPosition] != ' ' && remaining[breakPosition] != '_' && remaining[breakPosition] != '.') {
                breakPosition--;
            }
            if (breakPosition == 0) {
                breakPosition = fitLength;
            }
            out.emplace_back(remaining.substr(0, breakPosition));
            remaining.remove_prefix(breakPosition);
            if (!remaining.empty() && remaining.front() == ' ') {
                remaining.remove_prefix(1);
            }
        }
    }
}
