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

        void init(const std::filesystem::path& rootPath, std::unordered_map<ProjectConfig, std::string>& projectConfigs);

        void onStart() override;

        void onFixedUpdate() override;

        void onUpdate() override;

        void onStop() override;

        void shutdown() override;
    };
}
