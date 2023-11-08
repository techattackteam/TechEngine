#pragma once

#include "Panel.hpp"
#include "material/Material.hpp"
#include "renderer/Renderer.hpp"
#include "panels/materialEditorScene/Sphere.hpp"
#include "core/SceneCamera.hpp"

namespace TechEngine {
    class MaterialEditor : public Panel {
    private:
        Renderer &m_renderer;
        Material *m_material;
        std::string m_filepath;
        bool m_open = false;
        uint32_t frameBufferID;

        MaterialEditorGameObjects::Sphere m_sphere;
        SceneCamera m_camera;

    public:
        MaterialEditor(Renderer &renderer);

        ~MaterialEditor() = default;

        void onUpdate() override;

        void open(const std::string &name, const std::string &filepath);

        void renderObjectWithMaterial();

        void init();
    };
}
