#pragma once
#include "core/Core.hpp"
#include "app/Entry.hpp"
#include "ExportDLL.hpp"

namespace TechEngine {
    class CLIENT_DLL Client : public Core {
        Entry m_entry;

        friend class RuntimeClient;

    public:
        Client();

        void init() override;

        void onStart() override;

        void onFixedUpdate() override;

        void onUpdate() override;

        void onStop() override;

        void shutdown() override;
    };
}
