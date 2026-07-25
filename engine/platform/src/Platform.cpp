#include <TechEngine/base/Base.hpp>
#include <TechEngine/platform/Platform.hpp>

#include <GLFW/glfw3.h>

namespace TechEngine {
    const char* platformVersion() {
        (void)glfwGetVersionString();
        return baseVersion();
    }
}
