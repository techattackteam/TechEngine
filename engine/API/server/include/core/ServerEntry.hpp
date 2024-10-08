#pragma once
#include "common/include/core/Entry.hpp"


namespace TechEngineAPI {
    class ServerEntry : public Entry {
    private:
        inline static ServerEntry* instance = nullptr;

    public:
        ~ServerEntry() override;

        void init(TechEngine::SystemsRegistry* systemsRegistry) override;

        void shutdown() override;

        static ServerEntry* getInstance();

        static void deleteInstance();
    };
}
