#include <TechEngine/client/render/ImGuiHook.hpp>
#include <TechEngine/client/render/RenderView.hpp>

#include <ui/ViewTextureSwap.hpp>

#include <catch2/catch_test_macros.hpp>
#include <imgui.h>

#include <cstdint>
#include <vector>

using TechEngine::ImGuiViewTexture;
using TechEngine::VIEW_TEXTURE_TOKEN_BIT;
using TechEngine::ViewTextureSwap;

static constexpr ImTextureID SCENE_TOKEN = VIEW_TEXTURE_TOKEN_BIT | 1;
static constexpr ImTextureID GAME_TOKEN = VIEW_TEXTURE_TOKEN_BIT | 2;
static constexpr ImTextureID PLAIN_TEXTURE = 5;
static constexpr ImTextureID ATLAS_TEXTURE = 9;

// ImDrawData only points at its lists, so they live beside it.
struct DrawDataFixture {
    ImTextureData atlas;
    ImDrawList first{nullptr};
    ImDrawList second{nullptr};
    ImDrawData drawData;

    DrawDataFixture() {
        atlas.TexID = ATLAS_TEXTURE;
        addCommand(first, ImTextureRef(SCENE_TOKEN));
        addCommand(first, ImTextureRef(PLAIN_TEXTURE));
        ImTextureRef atlasRef;
        atlasRef._TexData = &atlas;
        addCommand(first, atlasRef);
        addCommand(second, ImTextureRef(GAME_TOKEN));
        addCommand(second, ImTextureRef(SCENE_TOKEN));
        drawData.CmdLists.push_back(&first);
        drawData.CmdLists.push_back(&second);
        drawData.CmdListsCount = drawData.CmdLists.Size;
    }

    static void addCommand(ImDrawList& list, const ImTextureRef texture) {
        ImDrawCmd command;
        command.TexRef = texture;
        command.ElemCount = 3;
        list.CmdBuffer.push_back(command);
    }

    std::vector<ImTextureID> textureIds() const {
        std::vector<ImTextureID> ids;
        for (const ImDrawList* list: drawData.CmdLists) {
            for (const ImDrawCmd& command: list->CmdBuffer) {
                ids.push_back(command.TexRef._TexID);
            }
        }
        return ids;
    }

    const ImDrawCmd& atlasCommand() const {
        return first.CmdBuffer[2];
    }
};

TEST_CASE("a command that names a view's token draws that view's texture until restore", "[editor][ui]") {
    DrawDataFixture fixture;
    ViewTextureSwap swap;
    const std::vector<ImGuiViewTexture> textures{ImGuiViewTexture{SCENE_TOKEN, 41}, ImGuiViewTexture{GAME_TOKEN, 42}};

    swap.apply(fixture.drawData, textures);

    const std::vector<ImTextureID> applied = fixture.textureIds();
    REQUIRE(applied.size() == 5);
    CHECK(applied[0] == 41);
    CHECK(applied[3] == 42);
    CHECK(applied[4] == 41);

    swap.restore();

    const std::vector<ImTextureID> restored = fixture.textureIds();
    CHECK(restored[0] == SCENE_TOKEN);
    CHECK(restored[3] == GAME_TOKEN);
    CHECK(restored[4] == SCENE_TOKEN);
}

TEST_CASE("a view token with no texture this frame draws texture 0", "[editor][ui]") {
    DrawDataFixture fixture;
    ViewTextureSwap swap;
    const std::vector<ImGuiViewTexture> sceneOnly{ImGuiViewTexture{SCENE_TOKEN, 41}};

    swap.apply(fixture.drawData, sceneOnly);

    const std::vector<ImTextureID> applied = fixture.textureIds();
    CHECK(applied[0] == 41);
    CHECK(applied[3] == 0);

    swap.restore();

    CHECK(fixture.textureIds()[3] == GAME_TOKEN);
}

TEST_CASE("a replayed snapshot finds a resized view's new texture", "[editor][ui]") {
    DrawDataFixture fixture;
    ViewTextureSwap swap;
    const std::vector<ImGuiViewTexture> beforeResize{ImGuiViewTexture{SCENE_TOKEN, 41}, ImGuiViewTexture{GAME_TOKEN, 42}};
    const std::vector<ImGuiViewTexture> afterResize{ImGuiViewTexture{SCENE_TOKEN, 77}, ImGuiViewTexture{GAME_TOKEN, 42}};

    swap.apply(fixture.drawData, beforeResize);
    swap.restore();
    swap.apply(fixture.drawData, afterResize);

    const std::vector<ImTextureID> applied = fixture.textureIds();
    CHECK(applied[0] == 77);
    CHECK(applied[4] == 77);
    CHECK(applied[3] == 42);
    swap.restore();
}

TEST_CASE("the swap leaves ImGui's atlas and a plain texture id alone", "[editor][ui]") {
    DrawDataFixture fixture;
    ViewTextureSwap swap;
    const std::vector<ImGuiViewTexture> textures{ImGuiViewTexture{SCENE_TOKEN, 41}, ImGuiViewTexture{GAME_TOKEN, 42}, ImGuiViewTexture{PLAIN_TEXTURE, 99}};

    swap.apply(fixture.drawData, textures);

    CHECK(fixture.textureIds()[1] == PLAIN_TEXTURE);
    CHECK(fixture.atlasCommand().TexRef._TexData == &fixture.atlas);
    CHECK(fixture.atlasCommand().GetTexID() == ATLAS_TEXTURE);

    swap.restore();

    CHECK(fixture.textureIds()[1] == PLAIN_TEXTURE);
    CHECK(fixture.atlasCommand().GetTexID() == ATLAS_TEXTURE);
}

TEST_CASE("restore without an apply, or twice, changes nothing", "[editor][ui]") {
    DrawDataFixture fixture;
    ViewTextureSwap swap;
    const std::vector<ImTextureID> original = fixture.textureIds();
    const std::vector<ImGuiViewTexture> textures{ImGuiViewTexture{SCENE_TOKEN, 41}};

    swap.restore();
    CHECK(fixture.textureIds() == original);

    swap.apply(fixture.drawData, textures);
    swap.restore();
    swap.restore();
    CHECK(fixture.textureIds() == original);
}
