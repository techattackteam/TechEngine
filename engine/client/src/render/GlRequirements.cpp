#include <TechEngine/base/diagnostics/Log.hpp>

#include <render/GlRequirements.hpp>

namespace TechEngine {
    bool meetsGlRequirements(const GlSupport& support) {
        bool meetsRequirements = true;
        if (support.loadedVersion < 40006 || !support.core46) {
            TE_LOGGER_ERROR("OpenGL 4.6 core profile support is required");
            meetsRequirements = false;
        }
        if (!support.bindlessTextures) {
            TE_LOGGER_ERROR("ARB_bindless_texture support is required");
            meetsRequirements = false;
        }
        return meetsRequirements;
    }
}
