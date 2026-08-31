#include "EditorApp.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {
    class EditorProbe : public TechEngine::EditorApp {
    public:
        TechEngine::Role loopRole() const {
            return m_loop.frame().role;
        }
    };
}

TEST_CASE("editor hosts a client", "[editor]") {
    const EditorProbe editor;

    REQUIRE(editor.loopRole() == TechEngine::Role::Client);
}
