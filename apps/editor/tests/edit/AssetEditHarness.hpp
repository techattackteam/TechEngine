#pragma once

#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/assets/AssetSource.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/mesh/MeshRenderer.hpp>
#include <TechEngine/core/resources/ResourceCache.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/core/scene/components/Name.hpp>

#include <assets/AssetIndex.hpp>
#include <assets/AssetTestFiles.hpp>
#include <edit/EditSession.hpp>
#include <edit/EditTestScene.hpp>
#include <edit/EditorSnapshot.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

namespace TechEngineTests {
    inline constexpr std::uint32_t MESH_RENDERER_MESH_FIELD = 0;

    inline constexpr TechEngine::ResourceId EDIT_SCENE_ID{0x5CE7E00000000001};
    inline constexpr TechEngine::ResourceId EDIT_OTHER_SCENE_ID{0x5CE7E00000000002};
    inline constexpr TechEngine::ResourceId EDIT_MESH_ID{0x3E54000000000001};
    inline constexpr TechEngine::ResourceId EDIT_SECOND_MESH_ID{0x3E54000000000002};
    inline constexpr TechEngine::ResourceId EDIT_BROKEN_MESH_ID{0x3E5400000000000B};
    inline constexpr TechEngine::ResourceId EDIT_SERVER_MESH_ID{0x3E5400000000005E};
    inline constexpr TechEngine::ResourceId EDIT_UNKNOWN_ID{0x3E540000000000FF};

    class MemoryAssetSource final : public TechEngine::AssetSource {
    private:
        std::unordered_map<TechEngine::ResourceId, std::vector<std::byte>> m_assets;

    public:
        void add(const TechEngine::ResourceId id, std::vector<std::byte> bytes) {
            m_assets[id] = std::move(bytes);
        }

        TechEngine::AssetAvailability find(const TechEngine::ResourceId id) const override {
            return m_assets.contains(id) ? TechEngine::AssetAvailability::Present : TechEngine::AssetAvailability::Unknown;
        }

        std::error_code read(const TechEngine::ResourceId id, std::vector<std::byte>& out) const override {
            out = m_assets.at(id);
            return {};
        }
    };

    inline TechEngine::ComponentRegistry& registerAssetEditComponents(TechEngine::ComponentRegistry& registry) {
        registerEditComponents(registry);
        registry.registerComponent<TechEngine::MeshRenderer>(TechEngine::MeshRenderer::tag);
        return registry;
    }

    // A scene held as a Ready cache entry, as the editor's startup load leaves it, beside an index and a source that
    // know two meshes, a mesh whose bytes do not parse, a server mesh and a second scene.
    class AssetEditHarness {
    private:
        TechEngine::ComponentRegistry m_registry;
        std::uint64_t m_selectionSequence = 0;

    public:
        TechEngine::Scene scene{registerAssetEditComponents(m_registry)};
        TechEngine::EditSession session;
        TechEngine::JobSystem jobs;
        MemoryAssetSource source;
        TechEngine::ResourceCache cache{jobs};
        TechEngine::AssetIndex index;
        TechEngine::ResourceHandle<TechEngine::SceneBytes> sceneHandle;

        AssetEditHarness() {
            cache.registerLoader<TechEngine::SceneBytes>(TechEngine::SCENE_KIND, TechEngine::SCENE_TYPE_TAG, &TechEngine::SceneBytes::loadPayload);
            cache.registerLoader<TechEngine::Mesh>(TechEngine::MESH_KIND, TechEngine::MESH_TYPE_TAG, &TechEngine::Mesh::loadPayload);
            cache.closeLoaders();
            cache.setSource(source);

            addAsset(EDIT_SCENE_ID, TechEngine::SCENE_TYPE_TAG, TechEngine::Side::Common, "assets/common/Edit.tescene", assetBytes(TechEngine::SCENE_KIND, EDIT_SCENE_ID, "scene"));
            addAsset(EDIT_OTHER_SCENE_ID, TechEngine::SCENE_TYPE_TAG, TechEngine::Side::Common, "assets/common/Other.tescene", assetBytes(TechEngine::SCENE_KIND, EDIT_OTHER_SCENE_ID, "scene"));
            addAsset(EDIT_MESH_ID, TechEngine::MESH_TYPE_TAG, TechEngine::Side::Client, "assets/client/Triangle.temesh", meshBytes(EDIT_MESH_ID));
            addAsset(EDIT_SECOND_MESH_ID, TechEngine::MESH_TYPE_TAG, TechEngine::Side::Client, "assets/client/Second.temesh", meshBytes(EDIT_SECOND_MESH_ID));
            addAsset(EDIT_BROKEN_MESH_ID, TechEngine::MESH_TYPE_TAG, TechEngine::Side::Client, "assets/client/Broken.temesh", assetBytes(TechEngine::MESH_KIND, EDIT_BROKEN_MESH_ID, "x"));
            addAsset(EDIT_SERVER_MESH_ID, TechEngine::MESH_TYPE_TAG, TechEngine::Side::Server, "assets/server/Collision.temesh", meshBytes(EDIT_SERVER_MESH_ID));

            REQUIRE(cache.request(EDIT_SCENE_ID, sceneHandle) == std::error_code{});
            REQUIRE(cache.runLoadingPhase() == std::error_code{});
            TechEngine::ResourceState state = TechEngine::ResourceState::Pending;
            REQUIRE(cache.getState(sceneHandle, state));
            REQUIRE(state == TechEngine::ResourceState::Ready);
        }

        ~AssetEditHarness() {
            if (sceneHandle.valid()) {
                cache.release(sceneHandle);
            }
            cache.drainFrees();
        }

        AssetEditHarness(const AssetEditHarness&) = delete;

        AssetEditHarness& operator=(const AssetEditHarness&) = delete;

        const TechEngine::ComponentRegistry& getRegistry() const {
            return m_registry;
        }

        TechEngine::Entity createWithMeshRenderer(const std::string_view name) {
            const TechEngine::Entity entity = scene.createEntity();
            REQUIRE(entity.valid());
            scene.getComponent<TechEngine::Name>(entity).value = std::string(name);
            scene.addComponent<TechEngine::MeshRenderer>(entity);
            return entity;
        }

        // Leaves the scene with no cache entry, as a failed startup load does.
        void releaseSceneEntry() {
            cache.release(sceneHandle);
            cache.drainFrees();
            sceneHandle = {};
        }

        void selectOnly(const TechEngine::Entity entity) {
            m_selectionSequence++;
            session.setSelection({entity}, entity, m_selectionSequence);
        }

        TechEngine::EditorSnapshot snapshot() const {
            TechEngine::EditorSnapshot out;
            session.buildSnapshot(scene, m_registry, out);
            return out;
        }

    private:
        void addAsset(const TechEngine::ResourceId id, const TechEngine::StringId type, const TechEngine::Side side, const std::string_view path, std::vector<std::byte> bytes) {
            TechEngine::AssetRecord record;
            record.id = id;
            record.typeTag = type;
            record.side = side;
            record.path = std::string(path);
            index.add(std::move(record));
            source.add(id, std::move(bytes));
        }

        static std::vector<std::byte> meshBytes(const TechEngine::ResourceId id) {
            TechEngine::Mesh mesh;
            mesh.id = id;
            mesh.positions = {TechEngine::Vec3(-0.5f, -0.5f, 0.0f), TechEngine::Vec3(0.5f, -0.5f, 0.0f), TechEngine::Vec3(0.0f, 0.5f, 0.0f)};
            mesh.indices = {0, 1, 2};
            mesh.slots = {TechEngine::MeshSlot{"Slot 0", 0, 3, TechEngine::ResourceId{}}};
            std::vector<std::byte> bytes;
            mesh.write(bytes);
            return bytes;
        }
    };
}
