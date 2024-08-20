#pragma once

#include <string>

#include "core/ExportDLL.hpp"
#include "systems/System.hpp"
#include <gl/glew.h>
#include <glfw/glfw3.h>

namespace TechEngine {
    class CLIENT_DLL Window : public System {
    private:
        SystemsRegistry& m_systemsRegistry;
        std::string m_title;
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        GLFWwindow* m_handler = nullptr;

    public:
        Window(SystemsRegistry& systemsRegistry);

        void init(const std::string& title, uint32_t width, uint32_t height);

        void onUpdate() override;

        void onFixedUpdate() override;

        void shutdown() override;

        void setTitle(const std::string& title);

        void setSize(uint32_t width, uint32_t height);

        uint32_t getWidth() const {
            return m_width;
        }

        uint32_t getHeight() const {
            return m_height;
        }

        float getAspect() const {
            return (float)m_width / (float)m_height;
        }
    };
}
