#pragma once
#include "ExportDLL.hpp"
#include "app/Entry.hpp"
#include "core/Core.hpp"

namespace TechEngine {
    class SERVER_DLL Server : public Core {
    private:
        Entry m_entry;
        friend class RuntimeServer;

    public:
        Server();

        void init() override;

        void onStart() override;

        void onFixedUpdate() override;

        void onUpdate() override;

        void onStop() override;

        void destroy() override;
    };
}
