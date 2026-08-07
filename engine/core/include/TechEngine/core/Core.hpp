#pragma once

namespace TechEngine {
    // `core` deliberately cannot see `client` — that is the client/server seam.
    const char* coreVersion();
}
