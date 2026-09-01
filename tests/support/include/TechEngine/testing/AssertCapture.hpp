#pragma once

#include <TechEngine/base/diagnostics/Assert.hpp>

#include <string>
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

    // GOTCHA: AssertContext's condition and message are views into a stack buffer inside
    // assertDispatch. They dangle the moment the throw unwinds it, so copy, never view.
    struct AssertFired {
        TechEngine::AssertKind kind;
        std::string condition;
        std::string message;
    };

    [[noreturn]] inline TechEngine::AssertResponse throwingHandler(const TechEngine::AssertContext& context) {
        throw AssertFired{context.kind, std::string{context.condition}, std::string{context.message}};
    }

    // The counting guard swallows a fatal assert, so the code under test runs on past a
    // failure it would never survive in production. This one makes the stop observable
    // instead: the throw unwinds exactly where abort() would have ended the process, so a
    // case asserts REQUIRE_THROWS_AS rather than inspecting a state that cannot exist.
    //
    // Two limits. The throw calls std::terminate if it has to cross a noexcept function or a
    // destructor, and any catch(...) between the check and the test swallows it.
    class FatalAssertGuard {
    public:
        FatalAssertGuard() : m_previous{TechEngine::setAssertHandler(&throwingHandler)} {
        }

        ~FatalAssertGuard() {
            TechEngine::setAssertHandler(m_previous);
        }

        FatalAssertGuard(const FatalAssertGuard&) = delete;

        FatalAssertGuard& operator=(const FatalAssertGuard&) = delete;

    private:
        TechEngine::AssertHandlerFn m_previous;
    };
}
