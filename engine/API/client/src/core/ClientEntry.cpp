#include "common/include/core/Entry.hpp"
#include "common/include/logger/Logger.hpp"

#include "client/include/core/ClientEntry.hpp"
#include "client/include/eventSystem/ClientEventSystem.hpp"
#include "client/include/ui/WidgetsManager.hpp"

#include "scene/Scene.hpp"
#include "script/ScriptEngine.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "ui/WidgetsRegistry.hpp"

namespace TechEngineAPI {
    ClientEntry::~ClientEntry() {
    }

    void ClientEntry::init(TechEngine::SystemsRegistry* systemsRegistry) {
        Entry::init(systemsRegistry);
        Logger::init("Client");
        ClientEventSystem::init(&systemsRegistry->getSystem<TechEngine::EventDispatcher>());
        WidgetsManager::init(&systemsRegistry->getSystem<TechEngine::WidgetsRegistry>(), &systemsRegistry->getSystem<TechEngine::EventDispatcher>());
    }

    void ClientEntry::shutdown() {
        Entry::shutdown();
        Logger::shutdown();
    }

    ClientEntry* ClientEntry::getInstance() {
        if (instance == nullptr) {
            instance = new ClientEntry();
        }
        return instance;
    }

    void ClientEntry::deleteInstance() {
        delete instance;
    }
}


extern "C" BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            TechEngine::ScriptEngine::setEntryPoint([](TechEngine::SystemsRegistry* systemsRegistry) {
                TechEngineAPI::ClientEntry::getInstance()->init(systemsRegistry);
                return TechEngineAPI::Logger::getDistSinks();
            });

            TechEngine::ScriptEngine::setExecuteExternalEventSystem([]() {
                TechEngineAPI::ClientEventSystem::execute();
            });
            break;
        case DLL_THREAD_ATTACH:
            // Code to run when a new thread is created in the process that loaded the DLL
            break;

        case DLL_THREAD_DETACH:
            // Code to run when a thread in the process that loaded the DLL exits
            break;

        case DLL_PROCESS_DETACH:
            // Code to run when the DLL is unloaded from a process's address space
            TechEngineAPI::ClientEntry::getInstance()->shutdown();
            break;
        default: break;
    }
    return TRUE;
}
