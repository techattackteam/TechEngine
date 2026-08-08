#pragma once

#include <TechEngine/base/diagnostics/Assert.hpp>

#include <vector>

namespace TechEngineTests {
    inline std::vector<TechEngine::AssertKind> g_fired;

    inline TechEngine::AssertResponse captureHandler(const TechEngine::AssertContext& context) {
        g_fired.push_back(context.kind);
        return TechEngine::AssertResponse{false, false};
    }

    // {false, false} keeps a fatal check from killing the runner, so a case can go on to
    // assert what the object looks like *after* a rejection.
    class AssertHandlerGuard {
    public:
        AssertHandlerGuard() : m_previous{TechEngine::setAssertHandler(&captureHandler)} {
            g_fired.clear();
        }

        ~AssertHandlerGuard() {
            TechEngine::setAssertHandler(m_previous);
            g_fired.clear();
        }

        AssertHandlerGuard(const AssertHandlerGuard&) = delete;

        AssertHandlerGuard& operator=(const AssertHandlerGuard&) = delete;

    private:
        TechEngine::AssertHandlerFn m_previous;
    };
}
