#pragma once

namespace TechEngine {
    // Exposes ONLY facade types whose full definitions ship here — never an
    // engine-private type (never a raw ResourceSystem*).
    // TODO(S3): placeholder until the real facade API lands.
    class ScriptContext {
    public:
        ScriptContext() = default;
    };
}
