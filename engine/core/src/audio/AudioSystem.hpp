#pragma once

#include "core/CoreExportDLL.hpp"
#include "systems/System.hpp"
#include "components/Components.hpp"
#include "components/Entity.hpp"


struct ma_engine;
struct ma_sound;

namespace TechEngine {
    class CORE_DLL AudioSystem : public System {
    private:
        SystemsRegistry& m_systemsRegistry;
        ma_engine* m_audioEngine;
        std::vector<ma_sound*> m_soundsToDelete;
        std::mutex m_soundsMutex;
        uint64_t m_frameCount = 0;

    public:
        AudioSystem(SystemsRegistry& m_systemsRegistry);

        ~AudioSystem() override;

        void init() override;

        void onStart() override;

        void onUpdate() override;

        void onFixedUpdate() override;

        void onStop() override;

        void shutdown() override;

        void playSound(const std::string& soundPath);

        void playSound3D(const std::string& soundPath, glm::vec3 position);

        void setListenerPosition(Entity entity, glm::vec3 position, glm::vec3 forward, glm::vec3 up);

    private:
        std::string translateErrorCodeToString(int errorCode);

        static void soundFinishCallback(void* userData, ma_sound* sound);

        void deleteSounds();
    };
}
