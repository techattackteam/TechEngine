#pragma once

#include "Panel.hpp"
#include "core/Client.hpp"
#include "core/SceneCamera.hpp"
#include "core/Server.hpp"
#include "materialEditorScene/Sphere.hpp"


namespace TechEngine {
    class MaterialEditor : public Panel {
    private:
        Client& client;
        Server& server;
        Material* m_material;

        std::string m_filepath;
        uint32_t frameBufferID;

        MaterialEditorGameObjects::Sphere m_sphere;
        SceneCamera m_camera;

    public:
        MaterialEditor(Client& client, Server& server);

        ~MaterialEditor() = default;

        void onUpdate() override;

        void open(const std::string& name, const std::string& filepath);

        void renderObjectWithMaterial();

        void init();
    };
}
