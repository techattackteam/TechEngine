#include "common/include/components/audio/Emitter.hpp"

#include "audio/AudioSystem.hpp"
#include "scene/Scene.hpp"

namespace TechEngineAPI {
    Emitter::Emitter(Entity entity,
                     TechEngine::AudioSystem* audioSystem,
                     TechEngine::Scene* scene): m_audioSystem(audioSystem),
                                                m_scene(scene),
                                                Component(entity) {
    }

    void Emitter::updateInternalPointer(TechEngine::Scene* scene) {
    }

    void Emitter::playSound(std::string& filepath) {
        m_audioSystem->playSound(filepath);
    }

    void Emitter::playSound3D(std::string& filepath) {
        TechEngine::Transform& transform = m_scene->getComponent<TechEngine::Transform>(m_entity);
        m_audioSystem->playSound3D(filepath, transform.m_position);
    }
}
