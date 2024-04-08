#pragma once

#include "Panel.hpp"
#include "material/Material.hpp"
#include "renderer/Renderer.hpp"
#include "core/SceneCamera.hpp"
#include "texture/TextureManager.hpp"
#include "material/MaterialManager.hpp"
#include "panels/materialEditorScene/Sphere.hpp"

namespace TechEngine {
    class MaterialEditor : public Panel {
    private:
        Renderer& m_renderer;
        Material* m_material;

        TextureManager& textureManager;
        MaterialManager& materialManager;
        Scene& scene;
        std::string m_filepath;
        uint32_t frameBufferID;

        MaterialEditorGameObjects::Sphere m_sphere;
        SceneCamera m_camera;

    public:
        MaterialEditor(Renderer& renderer, TextureManager& textureManager, MaterialManager& materialManager, Scene& scene);

        ~MaterialEditor() = default;

        void onUpdate() override;

        void open(const std::string& name, const std::string& filepath);

        void renderObjectWithMaterial();

        void init();
    };
}
