#pragma once
#include "Panel.hpp"

#include <RmlUi/Core/Context.h>

namespace TechEngine {
    class UIView : public Panel {
    private:
        uint32_t m_frameBufferID = 0;
        SystemsRegistry& m_appSystemsRegistry;
        Rml::Context* m_context = nullptr;

    public:
        UIView(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry);

        void onInit() override;

        void onUpdate() override;
    };
}
