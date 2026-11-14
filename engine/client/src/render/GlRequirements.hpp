#pragma once

namespace TechEngine {
    struct GlSupport {
        int loadedVersion = 0;
        bool core46 = false;
        bool bindlessTextures = false;
    };

    bool meetsGlRequirements(const GlSupport& support);
}
