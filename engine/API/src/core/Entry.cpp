#include "core/Entry.hpp"

#include <Windows.h>

#include "core/Timer.hpp"
#include "script/ScriptEngine.hpp"
#include "timer/Timer.hpp"

namespace TechEngineAPI {
    void Entry::init(TechEngine::SystemsRegistry* systemsRegistry) {
        Timer::init(&systemsRegistry->getSystem<TechEngine::Timer>());
    }

    void Entry::shutdown() {
        Timer::shutdown();
    }
}


extern "C" BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            TechEngine::ScriptEngine::setEntryPoint([](TechEngine::SystemsRegistry* systemsRegistry) {
                TechEngineAPI::Entry::init(systemsRegistry);
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
            break;
        default: break;
    }
    return TRUE;
}
