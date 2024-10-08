#include "common/include/core/Entry.hpp"
#include "common/include/logger/Logger.hpp"

#include "server/include/core/ServerEntry.hpp"

#include "script/ScriptEngine.hpp"

namespace TechEngineAPI {
    ServerEntry::~ServerEntry() {
    }

    void ServerEntry::init(TechEngine::SystemsRegistry* systemsRegistry) {
        Entry::init(systemsRegistry);
        Logger::init("Server");
    }

    void ServerEntry::shutdown() {
        Entry::shutdown();
        Logger::shutdown();
    }

    ServerEntry* ServerEntry::getInstance() {
        if (instance == nullptr) {
            instance = new ServerEntry();
        }
        return instance;
    }

    void ServerEntry::deleteInstance() {
        delete instance;
    }
}


extern "C" BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            TechEngine::ScriptEngine::setEntryPoint([](TechEngine::SystemsRegistry* systemsRegistry) {
                TechEngineAPI::ServerEntry::getInstance()->init(systemsRegistry);
                return TechEngineAPI::Logger::getDistSinks();
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
            TechEngineAPI::ServerEntry::getInstance()->shutdown();
            break;
        default: break;
    }
    return TRUE;
}
