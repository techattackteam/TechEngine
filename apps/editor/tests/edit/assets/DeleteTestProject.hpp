#pragma once

#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/mesh/MeshRenderer.hpp>
#include <TechEngine/core/model/Model.hpp>
#include <TechEngine/core/resources/AssetRef.hpp>
#include <TechEngine/core/resources/ResourceCache.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/core/texture/Texture.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/MountTable.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <assets/index/AssetIndex.hpp>
#include <assets/index/AssetTestFiles.hpp>
#include <edit/scene/AssetEditHarness.hpp>
#include <edit/session/EditSession.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

namespace TechEngineTests {
    inline constexpr TechEngine::ResourceId DELETE_OPEN_SCENE_ID{0xDE1E000000000001};
    inline constexpr TechEngine::ResourceId DELETE_OTHER_SCENE_ID{0xDE1E000000000002};
    inline constexpr TechEngine::ResourceId DELETE_MESH_ID{0xDE1E000000000010};
    inline constexpr TechEngine::ResourceId DELETE_KEPT_MESH_ID{0xDE1E000000000011};
    inline constexpr TechEngine::ResourceId DELETE_PAINT_ID{0xDE1E000000000020};
    inline constexpr TechEngine::ResourceId DELETE_BRICK_ID{0xDE1E000000000030};
    inline constexpr TechEngine::ResourceId DELETE_CRATE_MODEL_ID{0xDE1E000000000040};
    inline constexpr TechEngine::ResourceId DELETE_CRATE_MESH_ID{0xDE1E000000000041};
    inline constexpr TechEngine::ResourceId DELETE_CRATE_MATERIAL_ID{0xDE1E000000000042};

    inline constexpr std::string_view DELETE_ASSETS_ROOT = "project://assets";
    inline constexpr std::string_view DELETE_OPEN_SCENE = "assets/common/Scenes/Open.tescene";
    inline constexpr std::string_view DELETE_SCENES_FOLDER = "assets/common/Scenes";
    inline constexpr std::string_view DELETE_OTHER_SCENE = "assets/common/Other.tescene";
    inline constexpr std::string_view DELETE_MESH = "assets/client/Mesh.temesh";
    inline constexpr std::string_view DELETE_KEPT_MESH = "assets/client/Kept.temesh";
    inline constexpr std::string_view DELETE_PAINT = "assets/client/Paint.tematerial";
    inline constexpr std::string_view DELETE_BRICK = "assets/client/Brick.tetexture";
    inline constexpr std::string_view DELETE_CRATE_SOURCE = "assets/client/Crate.gltf";
    inline constexpr std::string_view DELETE_CRATE_MODEL = "assets/client/Crate.temodel";
    inline constexpr std::string_view DELETE_CRATE_FOLDER = "assets/client/Crate";
    inline constexpr std::string_view DELETE_CRATE_MESH = "assets/client/Crate/Crate.temesh";
    inline constexpr std::string_view DELETE_CRATE_MATERIAL = "assets/client/Crate/Crate.tematerial";

    inline std::string projectPath(const std::string_view relative) {
        return "project://" + std::string(relative);
    }

    // On disk, with who names whom:
    //   Scenes/Open.tescene, the open scene, names Mesh, Paint and Kept, as the live scene does.
    //   Other.tescene names Mesh. Mesh.temesh's slot defaults to Paint, and Paint names Brick.
    //   Crate.temodel names Crate/Crate.temesh, whose slot defaults to Crate/Crate.tematerial.
    //   Crate.gltf is the source Crate/ was created from.
    class DeleteTestProject {
    private:
        ScratchDirectory m_scratch;
        TechEngine::MountTable m_mounts;
        TechEngine::ComponentRegistry m_registry;
        std::atomic<bool> m_createReleased = false;

    public:
        TechEngine::FileAccess files;
        TechEngine::Scene scene;
        TechEngine::EditSession session;
        TechEngine::JobSystem jobs;
        TechEngine::ResourceCache cache;
        TechEngine::AssetIndex index;
        std::vector<TechEngine::BatchId> createBatches;
        TechEngine::Entity painted;
        TechEngine::Entity kept;

        explicit DeleteTestProject(const std::string_view name) : m_scratch(name), files(m_mounts), scene(registerAssetEditComponents(m_registry)), cache(jobs) {
            m_scratch.makeDirectory("assets/common");
            m_scratch.makeDirectory("assets/client");
            m_scratch.makeDirectory("assets/server");
            m_mounts.mount("project", m_scratch.root());

            cache.registerLoader<TechEngine::SceneBytes>(TechEngine::SCENE_KIND, TechEngine::SCENE_TYPE_TAG, &TechEngine::SceneBytes::loadPayload);
            cache.registerLoader<TechEngine::Mesh>(TechEngine::MESH_KIND, TechEngine::MESH_TYPE_TAG, &TechEngine::Mesh::loadPayload);
            cache.registerLoader<TechEngine::Texture>(TechEngine::TEXTURE_KIND, TechEngine::TEXTURE_TYPE_TAG, &TechEngine::Texture::loadPayload);
            cache.registerLoader<TechEngine::Material>(TechEngine::MATERIAL_KIND, TechEngine::MATERIAL_TYPE_TAG, &TechEngine::Material::loadPayload);
            cache.closeLoaders();

            painted = addRenderer(scene, DELETE_MESH_ID, DELETE_PAINT_ID);
            kept = addRenderer(scene, DELETE_KEPT_MESH_ID, TechEngine::ResourceId{});
            writeScene(DELETE_OPEN_SCENE, DELETE_OPEN_SCENE_ID, scene);

            TechEngine::Scene other(m_registry);
            addRenderer(other, DELETE_MESH_ID, TechEngine::ResourceId{});
            writeScene(DELETE_OTHER_SCENE, DELETE_OTHER_SCENE_ID, other);

            writeMesh(DELETE_MESH, DELETE_MESH_ID, DELETE_PAINT_ID);
            writeMesh(DELETE_KEPT_MESH, DELETE_KEPT_MESH_ID, TechEngine::ResourceId{});
            writeMesh(DELETE_CRATE_MESH, DELETE_CRATE_MESH_ID, DELETE_CRATE_MATERIAL_ID);
            writeMaterial(DELETE_PAINT, DELETE_PAINT_ID, DELETE_BRICK_ID);
            writeMaterial(DELETE_CRATE_MATERIAL, DELETE_CRATE_MATERIAL_ID, TechEngine::ResourceId{});
            writeBytes(m_scratch, DELETE_BRICK, assetBytes(TechEngine::TEXTURE_KIND, DELETE_BRICK_ID, "texture"));
            writeModel(DELETE_CRATE_MODEL, DELETE_CRATE_MODEL_ID, DELETE_CRATE_MESH_ID);
            m_scratch.writeFile(DELETE_CRATE_SOURCE, "{}");

            REQUIRE(index.build(files, DELETE_ASSETS_ROOT) == std::error_code{});
            session.setSceneFile(TechEngine::SceneFile{DELETE_OPEN_SCENE_ID, projectPath(DELETE_OPEN_SCENE)});
        }

        ~DeleteTestProject() {
            releaseCreate();
            for (const TechEngine::BatchId batch: createBatches) {
                jobs.wait(batch);
            }
        }

        DeleteTestProject(const DeleteTestProject&) = delete;

        DeleteTestProject& operator=(const DeleteTestProject&) = delete;

        TechEngine::ComponentRegistry& getRegistry() {
            return m_registry;
        }

        bool exists(const std::string_view relative) const {
            return std::filesystem::exists(m_scratch.root() / relative);
        }

        bool isIndexed(const TechEngine::ResourceId id) const {
            TechEngine::AssetRecord record;
            return index.find(id, record);
        }

        const TechEngine::MeshRenderer& rendererOf(const TechEngine::Entity entity) const {
            return scene.getComponent<TechEngine::MeshRenderer>(entity);
        }

        void startCreate() {
            std::array<TechEngine::Task, 1> tasks{[this] {
                while (!m_createReleased.load()) {
                    std::this_thread::yield();
                }
            }};
            createBatches.push_back(jobs.submit(tasks));
        }

        void releaseCreate() {
            m_createReleased.store(true);
        }

    private:
        static TechEngine::Entity addRenderer(TechEngine::Scene& target, const TechEngine::ResourceId mesh, const TechEngine::ResourceId material) {
            const TechEngine::Entity entity = target.createEntity();
            REQUIRE(entity.valid());
            TechEngine::MeshRenderer renderer;
            renderer.mesh.id = mesh;
            if (material.valid()) {
                renderer.slots.push_back(TechEngine::MaterialSlot{"Slot 0", TechEngine::AssetRef<TechEngine::Material>{material, {}}});
            }
            target.addComponent<TechEngine::MeshRenderer>(entity, std::move(renderer));
            return entity;
        }

        void writeScene(const std::string_view relative, const TechEngine::ResourceId id, const TechEngine::Scene& source) const {
            TechEngine::SceneAsset asset;
            asset.id = id;
            std::vector<std::byte> bytes;
            asset.write(source, bytes);
            writeBytes(m_scratch, relative, bytes);
        }

        void writeMesh(const std::string_view relative, const TechEngine::ResourceId id, const TechEngine::ResourceId defaultMaterial) const {
            TechEngine::Mesh mesh;
            mesh.id = id;
            mesh.positions = {TechEngine::Vec3(-0.5f, -0.5f, 0.0f), TechEngine::Vec3(0.5f, -0.5f, 0.0f), TechEngine::Vec3(0.0f, 0.5f, 0.0f)};
            mesh.indices = {0, 1, 2};
            mesh.slots = {TechEngine::MeshSlot{"Slot 0", 0, 3, defaultMaterial}};
            std::vector<std::byte> bytes;
            mesh.write(bytes);
            writeBytes(m_scratch, relative, bytes);
        }

        void writeMaterial(const std::string_view relative, const TechEngine::ResourceId id, const TechEngine::ResourceId baseColor) const {
            TechEngine::Material material;
            material.id = id;
            material.baseColorTexture.id = baseColor;
            std::vector<std::byte> bytes;
            material.write(bytes);
            writeBytes(m_scratch, relative, bytes);
        }

        void writeModel(const std::string_view relative, const TechEngine::ResourceId id, const TechEngine::ResourceId mesh) const {
            TechEngine::Model model;
            model.id = id;
            TechEngine::ModelNode& node = model.nodes.emplace_back();
            node.name = "Crate";
            node.mesh = mesh;
            std::vector<std::byte> bytes;
            model.write(bytes);
            writeBytes(m_scratch, relative, bytes);
        }
    };
}
