#pragma once
#include <complex.h>
#include <common/include/components/Component.hpp>

namespace TechEngine {
    class AudioSystem;
}

namespace TechEngineAPI {
    class API_DLL Emitter : public Component {
    private:
        TechEngine::AudioSystem* m_audioSystem = nullptr;
        TechEngine::Scene* m_scene = nullptr;

    public:
        Emitter(Entity entity, TechEngine::AudioSystem* audioSystem, TechEngine::Scene* scene);

        void updateInternalPointer(TechEngine::Scene* scene) override;
        
        void playSound(std::string& filepath);

        void playSound3D(std::string& filepath);
    };
}
