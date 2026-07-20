// SDK smoke — the F11 acid test, automated (ADR-008 §7).
//
// Includes ONLY the public SDK surface (<TechEngine/sdk/...>) and links
// TechEngine::sdk ALONE — no engine module. If any engine-private type ever leaks
// into the SDK surface, this translation unit fails to compile and breaks the
// build the day the leak is introduced.

#include <TechEngine/sdk/ScriptContext.hpp>

int main() {
    TechEngine::ScriptContext ctx;
    (void)ctx;
    return 0;
}
