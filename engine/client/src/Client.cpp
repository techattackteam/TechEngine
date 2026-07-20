#include <TechEngine/client/Client.hpp>

#include <TechEngine/base/Base.hpp>
#include <TechEngine/core/Core.hpp>
#include <TechEngine/platform/Platform.hpp>

#include <miniaudio.h>

namespace TechEngine {
    const char* clientVersion() {
        (void)baseVersion();
        (void)coreVersion();
        (void)platformVersion();
        return "0.0.0";
    }
}
