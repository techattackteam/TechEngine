#pragma once

namespace TechEngine {
    // Placeholder anchor for the `base` module (ADR-006 §1: math, logging, assert,
    // clock, small containers). Exists so the empty skeleton links and the public
    // include path <TechEngine/base/Base.hpp> is exercised from the first commit.
    // Replaced by the real base surface as it lands.
    const char* baseVersion();
}
