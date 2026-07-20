#include <TechEngine/core/Core.hpp>

#include <TechEngine/base/Base.hpp>
#include <TechEngine/platform/Platform.hpp>

#include <toml++/toml.hpp>

namespace TechEngine {
    const char* coreVersion() {
        (void)baseVersion();
        (void)platformVersion();
        return "0.0.0";
    }
}
