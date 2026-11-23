#include <TechEngine/app/App.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/core/resources/Side.hpp>

#include <assets/AssetIndex.hpp>
#include <edit/EditSession.hpp>
#include <edit/EditorCommands.hpp>
#include <edit/EditorSnapshot.hpp>
#include <ui/panels/AssetBrowserPanel.hpp>

#include <catch2/catch_test_macros.hpp>
#include <imgui.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

using TechEngine::AssetBrowserPanel;
using TechEngine::EditorSnapshot;
using TechEngine::ListedEntry;
using TechEngine::ListedKind;
using TechEngine::Side;

static constexpr float FRAME = 1.0f / 60.0f;
static constexpr std::string_view CLIENT_FOLDER = "project://assets/client";
static constexpr std::string_view SPONZA_FOLDER = "project://assets/client/Sponza";
static constexpr std::string_view MESHES_FOLDER = "project://assets/client/Sponza/Meshes";

static std::string rememberedLayout(const std::string_view folder) {
    return "[AssetBrowser][Panel]\nOpenFolder=" + std::string(folder) + "\n";
}

static EditorSnapshot listedFolders(const std::uint64_t revision, const std::initializer_list<std::string_view> folders) {
    EditorSnapshot snapshot;
    snapshot.sideFolders = {
        {Side::Common, "project://assets/common"},
        {Side::Client, std::string(CLIENT_FOLDER)},
        {Side::Server, "project://assets/server"},
    };
    for (const std::string_view folder: folders) {
        ListedEntry& entry = snapshot.listing.emplace_back();
        entry.path = std::string(folder);
        entry.side = Side::Client;
        entry.kind = ListedKind::Folder;
    }
    std::ranges::sort(snapshot.listing, {}, &ListedEntry::path);
    snapshot.listingRevision = revision;
    return snapshot;
}

class BrowserAppProbe final : public TechEngine::App {
public:
    // Never run, so a command the panel sends is refused and logged.
    TechEngine::EditSession session;
    TechEngine::AssetIndex assets;
    std::vector<TechEngine::BatchId> importBatches;
    TechEngine::EditorCommands commands{*this, session, m_registry, m_files, m_resources, m_sceneHandle, assets, m_jobs, importBatches};

    BrowserAppProbe() : App(TechEngine::Role::DedicatedServer) {
    }

protected:
    void init() override {
    }
};

struct HeadlessBrowser {
    ImGuiContext* previous = nullptr;
    ImGuiContext* context = nullptr;
    BrowserAppProbe app;
    AssetBrowserPanel panel{app.commands};

    explicit HeadlessBrowser(const std::string_view layout = {}) {
        previous = ImGui::GetCurrentContext();
        context = ImGui::CreateContext();
        // CreateContext leaves an earlier context current.
        ImGui::SetCurrentContext(context);
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.DisplaySize = ImVec2(1024.0f, 768.0f);
        io.DeltaTime = FRAME;
        io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
        panel.registerSettings();
        if (!layout.empty()) {
            ImGui::LoadIniSettingsFromMemory(layout.data(), layout.size());
        }
    }

    ~HeadlessBrowser() {
        ImGui::DestroyContext(context);
        ImGui::SetCurrentContext(previous);
    }

    HeadlessBrowser(const HeadlessBrowser&) = delete;
    HeadlessBrowser& operator=(const HeadlessBrowser&) = delete;

    void frame(const EditorSnapshot& snapshot) {
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(640.0f, 480.0f));
        panel.draw(snapshot);
        ImGui::Render();
    }

    std::string savedLayout() const {
        std::size_t size = 0;
        const char* text = ImGui::SaveIniSettingsToMemory(&size);
        return std::string(text, size);
    }
};

TEST_CASE("the remembered open folder is read from the layout before the first frame", "[editor][ui][browser]") {
    const HeadlessBrowser browser(rememberedLayout(SPONZA_FOLDER));

    CHECK(browser.panel.getOpenFolder() == SPONZA_FOLDER);
}

TEST_CASE("with no remembered folder the browser opens the client side once the first listing arrives", "[editor][ui][browser]") {
    HeadlessBrowser browser;

    browser.frame(listedFolders(1, {SPONZA_FOLDER}));

    CHECK(browser.panel.getOpenFolder() == CLIENT_FOLDER);
}

TEST_CASE("a remembered folder waits for the first listing before it is checked", "[editor][ui][browser]") {
    HeadlessBrowser browser(rememberedLayout(MESHES_FOLDER));

    browser.frame(EditorSnapshot{});
    browser.frame(EditorSnapshot{});
    CHECK(browser.panel.getOpenFolder() == MESHES_FOLDER);

    browser.frame(listedFolders(1, {SPONZA_FOLDER, MESHES_FOLDER}));
    CHECK(browser.panel.getOpenFolder() == MESHES_FOLDER);
}

TEST_CASE("a remembered folder that is gone opens its nearest listed ancestor", "[editor][ui][browser]") {
    HeadlessBrowser browser(rememberedLayout(MESHES_FOLDER));

    browser.frame(listedFolders(1, {SPONZA_FOLDER}));

    CHECK(browser.panel.getOpenFolder() == SPONZA_FOLDER);
}

TEST_CASE("a remembered folder under no side opens the client side", "[editor][ui][browser]") {
    HeadlessBrowser browser(rememberedLayout("project://assets/clientele/Sponza"));

    browser.frame(listedFolders(1, {SPONZA_FOLDER}));

    CHECK(browser.panel.getOpenFolder() == CLIENT_FOLDER);
}

TEST_CASE("a refresh that removes the open folder opens its nearest listed ancestor", "[editor][ui][browser]") {
    HeadlessBrowser browser(rememberedLayout(MESHES_FOLDER));
    browser.frame(listedFolders(1, {SPONZA_FOLDER, MESHES_FOLDER}));
    REQUIRE(browser.panel.getOpenFolder() == MESHES_FOLDER);

    browser.frame(listedFolders(2, {SPONZA_FOLDER}));

    CHECK(browser.panel.getOpenFolder() == SPONZA_FOLDER);
}

TEST_CASE("the open folder is saved in the layout and read back from it", "[editor][ui][browser]") {
    std::string saved;
    {
        HeadlessBrowser browser(rememberedLayout(MESHES_FOLDER));
        browser.frame(listedFolders(1, {SPONZA_FOLDER}));
        REQUIRE(browser.panel.getOpenFolder() == SPONZA_FOLDER);
        saved = browser.savedLayout();
    }

    CHECK(saved.find("[AssetBrowser][Panel]") != std::string::npos);
    CHECK(saved.find("OpenFolder=" + std::string(SPONZA_FOLDER)) != std::string::npos);
    const HeadlessBrowser reopened(saved);
    CHECK(reopened.panel.getOpenFolder() == SPONZA_FOLDER);
}
