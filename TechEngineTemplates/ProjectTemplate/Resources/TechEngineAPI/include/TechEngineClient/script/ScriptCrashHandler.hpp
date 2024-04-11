#pragma once

#include "core/Logger.hpp"
#include <windows.h>
#include <excpt.h>
#include <dbghelp.h>
#include "events/scripts/ScriptCrashEvent.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "script/Script.hpp"
#pragma comment(lib, "Dbghelp.lib")

namespace TechEngine {
    inline void logException(_EXCEPTION_POINTERS* exceptionRecord, const std::string& scriptName) {
        TE_LOGGER_ERROR("Script {0} throw exception code: 0x{1}", scriptName, std::to_string(exceptionRecord->ExceptionRecord->ExceptionCode));
        switch (exceptionRecord->ExceptionRecord->ExceptionCode) {
            case EXCEPTION_ACCESS_VIOLATION:
                TE_LOGGER_ERROR("\tAccess Violation at address: 0x{0}", exceptionRecord->ExceptionRecord->ExceptionInformation[1]);
                break;
            case EXCEPTION_INT_DIVIDE_BY_ZERO:
                TE_LOGGER_ERROR("\tInteger Divide by Zero\n");
                break;
            case EXCEPTION_STACK_OVERFLOW:
                TE_LOGGER_ERROR("\tStack Overflow\n");
                break;
            case EXCEPTION_FLT_DIVIDE_BY_ZERO:
                TE_LOGGER_ERROR("\tFloating Point Divide by Zero\n");
                break;
            case EXCEPTION_FLT_OVERFLOW:
                TE_LOGGER_ERROR("\tFloating Point Overflow\n");
                break;
            case EXCEPTION_FLT_UNDERFLOW:
                TE_LOGGER_ERROR("\tFloating Point Underflow\n");
                break;
            case EXCEPTION_FLT_INEXACT_RESULT:
                TE_LOGGER_ERROR("\tFloating Point Inexact Result\n");
                break;
            case EXCEPTION_FLT_INVALID_OPERATION:
                TE_LOGGER_ERROR("\tFloating Point Invalid Operation\n");
                break;
            default:
                TE_LOGGER_ERROR("\tUnhandled Exception\n");
        }
        std::vector<void*> stackFrames(63);
        USHORT frames = CaptureStackBackTrace(0, stackFrames.size(), stackFrames.data(), nullptr);

        HANDLE process = GetCurrentProcess();
        SymInitialize(process, nullptr, TRUE);

        char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        PSYMBOL_INFO symbol = reinterpret_cast<PSYMBOL_INFO>(buffer);
        symbol->MaxNameLen = MAX_SYM_NAME;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

        for (USHORT i = 0; i < frames; i++) {
            DWORD displacement = 0; // Initialize displacement
            SymFromAddr(process, reinterpret_cast<DWORD64>(stackFrames[i]), 0, symbol);

            IMAGEHLP_LINE64 lineInfo;
            ZeroMemory(&lineInfo, sizeof(lineInfo));
            lineInfo.SizeOfStruct = sizeof(lineInfo);

            if (SymGetLineFromAddr64(process, reinterpret_cast<DWORD64>(stackFrames[i]), &displacement, &lineInfo)) {
                TE_LOGGER_ERROR("\t{0}: {1} - 0x{2} (File: {3}, Line: {4})", i, symbol->Name, symbol->Address, lineInfo.FileName, lineInfo.LineNumber);
            } else {
                // Existing logging if line info is unavailable
                TE_LOGGER_ERROR("\t{0}: {1} - 0x{2}", i, symbol->Name, symbol->Address);
            }
        }

        SymCleanup(process);
    }

    inline int filter(unsigned int code, struct _EXCEPTION_POINTERS* ep, Script* script) {
        std::string name;
        if (script != nullptr) {
            name = script->getName();
        } else {
            name = "";
        }
        logException(ep, name);
        EventDispatcher::getInstance().dispatch(new ScriptCrashEvent());
        if (code == EXCEPTION_ACCESS_VIOLATION) {
            return EXCEPTION_EXECUTE_HANDLER;
        } else {
            return EXCEPTION_CONTINUE_SEARCH;
        };
    }

#define CATCH_EXCEPTION_IN_FUNCTION(callback) \
    [callback](Event* event) {\
    __try {\
        callback(event);\
    } __except (filter(GetExceptionCode(), GetExceptionInformation(), nullptr)) {\
    }\
}
#define RUN_SCRIPT_FUNCTION(script, func) \
    if (runtime) { \
        script->func(); \
    } else { \
        __try { \
            script->func(); \
        } __except (filter(GetExceptionCode(), GetExceptionInformation(), script)) { \
    } \
}
}
