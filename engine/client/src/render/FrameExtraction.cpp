#include <TechEngine/client/render/FrameExtraction.hpp>
#include <TechEngine/client/render/FramePacket.hpp>
#include <TechEngine/core/camera/Camera.hpp>
#include <TechEngine/core/mesh/MeshRenderer.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>

#include <cstdint>

namespace TechEngine {
    void FrameExtraction::extractFramePacket(const Scene& scene, bool& warnedNoCamera, FramePacket& out) {
        out.instances.clear();
        out.materials.clear();
        if (scene.isRegistered(componentTypeId<MeshRenderer>())) {
            auto query = scene.query<Write<>, Read<Transform, MeshRenderer>>();
            query.each([&](const Entity entity, const Transform& transform, const MeshRenderer& meshRenderer) {
                const std::uint32_t firstMaterial = static_cast<std::uint32_t>(out.materials.size());
                out.instances.push_back(PacketInstance{
                    .entity = entity,
                    .world = transform.getWorldMatrix(),
                    .mesh = meshRenderer.mesh.handle,
                    .firstMaterial = firstMaterial,
                    .materialCount = static_cast<std::uint32_t>(meshRenderer.slots.size()),
                });
                for (const auto& material: meshRenderer.slots) {
                    out.materials.push_back(material.material.handle);
                }
            });
        }

        Entity cameraEntity = Entity{};
        out.hasCamera = Camera::findMainCamera(scene, cameraEntity, warnedNoCamera);
        if (out.hasCamera) {
            out.camera.world = scene.getComponent<Transform>(cameraEntity).getWorldMatrix();
            out.camera.settings = scene.getComponent<Camera>(cameraEntity);
        }
    }
}
