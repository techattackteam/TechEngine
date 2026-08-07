#pragma once

namespace TechEngine {
    // Does nothing, and must still be called. `app` is a static lib, so the object file
    // holding the operator new/delete replacements is only pulled into the exe if something
    // in it resolves an undefined symbol — and a dropped replacement is a memory profile
    // that is quietly, partially wrong rather than absent.
    void memoryTrackingAnchor();
}
