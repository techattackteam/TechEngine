#include <TechEngine/client/render/RenderView.hpp>

#include <ui/ViewTextureSwap.hpp>

#include <cstdint>

namespace TechEngine {
    void ViewTextureSwap::apply(ImDrawData& drawData, const std::span<const ImGuiViewTexture> viewTextures) {
        for (ImDrawList* list: drawData.CmdLists) {
            if (list == nullptr) {
                continue;
            }
            for (ImDrawCmd& command: list->CmdBuffer) {
                if (command.TexRef._TexData != nullptr) {
                    continue;
                }
                const std::uint64_t token = command.TexRef._TexID;
                const bool isViewToken = (token & VIEW_TEXTURE_TOKEN_BIT) != 0;
                if (!isViewToken) {
                    continue;
                }
                m_swapped.push_back(SwappedViewCommand{
                    .command = &command,
                    .token = token,
                });
                std::uint64_t nativeTexture = 0;
                for (const ImGuiViewTexture& texture: viewTextures) {
                    if (texture.token == token) {
                        nativeTexture = texture.nativeTexture;
                        break;
                    }
                }
                command.TexRef._TexID = nativeTexture;
            }
        }
    }

    void ViewTextureSwap::restore() {
        for (const SwappedViewCommand& swapped: m_swapped) {
            if (swapped.command != nullptr) {
                swapped.command->TexRef._TexID = swapped.token;
            }
        }
        m_swapped.clear();
    }
}
