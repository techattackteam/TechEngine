#pragma once

#include <string_view>

namespace TechEngine {
    struct ProgramDescription {
        std::string_view vertexSource;
        std::string_view fragmentSource;
    };

    struct GpuProgram {
        unsigned int id = 0;
    };
}
