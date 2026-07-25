#include <TechEngine/base/Base.hpp>
#include <TechEngine/client/Client.hpp>
#include <TechEngine/core/Core.hpp>
#include <TechEngine/platform/Platform.hpp>

namespace TechEngine {
    const char* clientVersion() {
        (void)baseVersion();
        (void)coreVersion();
        (void)platformVersion();
        return "0.0.0";
    }
}
