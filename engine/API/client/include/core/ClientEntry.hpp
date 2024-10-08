#pragma once

#include "common/include/core/Entry.hpp"

namespace TechEngineAPI {
    class ClientEntry : public Entry {
        inline static ClientEntry* instance = nullptr;

    public:
        ~ClientEntry() override;

        void init(TechEngine::SystemsRegistry* systemsRegistry) override;

        void shutdown() override;

        static ClientEntry* getInstance();

        static void deleteInstance();
    };
}
