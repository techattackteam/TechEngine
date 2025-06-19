#pragma once
#include "systems/System.hpp"
#include "core/CoreExportDLL.hpp"

#include <string>

#include "components/Components.hpp"
#include "glm/fwd.hpp"
#include <list>

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

        void setListenerPosition(int entity, glm::vec3 position, glm::vec3 forward, glm::vec3 up);

    private:
        std::string translateErrorCodeToString(int errorCode);

        static void soundFinishCallback(void* userData, ma_sound* sound);

        void deleteSounds();
    };
}
