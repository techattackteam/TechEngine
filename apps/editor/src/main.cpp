#include <TechEngine/app/App.hpp>
#include <TechEngine/base/Assert.hpp>
#include <TechEngine/base/Log.hpp>

int main() {
    TechEngine::initLogging();
    TE_ENSURE(false, "smoke test - non-fatal path");

    // fatal: logs Critical, flushes, then aborts the process
    TE_CHECK(false, "smoke test - fatal path");
}
