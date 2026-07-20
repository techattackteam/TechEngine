#pragma once

namespace TechEngine {
    // Placeholder SDK facade (ADR-006 §3): the curated public surface a game/script
    // module compiles against. Exposes ONLY facade types whose full definitions ship
    // here — never an engine-private type (never a raw ResourceSystem*). The real
    // facade API lands with the scripting ADR.
    class ScriptContext {
      public:
        ScriptContext() = default;
    };
}
