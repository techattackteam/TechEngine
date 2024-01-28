#pragma once

#include <cstdint>
#include "core/Core.hpp"

namespace TechEngine {
    class /*Engine_API*/ RendererSettings {
    public:

        static inline uint32_t targetId = 0;
        static inline uint32_t width = 1280;
        static inline uint32_t height = 720;
        static inline float aspectRatio = 2;

        static inline void changeTarget(uint32_t targetId) {
            RendererSettings::targetId = targetId;
        }

        static inline void changeTarget(int32_t targetId, uint32_t width, uint32_t height) {
            RendererSettings::targetId = targetId;
            resize(width, height);
        }

        static inline void resize(uint32_t width, uint32_t height) {
            RendererSettings::width = width;
            RendererSettings::height = height;
            RendererSettings::aspectRatio = (float) width / (float) height;
        }
    };
}