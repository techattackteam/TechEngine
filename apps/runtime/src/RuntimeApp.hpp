#pragma once

#include <TechEngine/clientapp/ClientApp.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/Scene.hpp>

#include <cstddef>
#include <filesystem>
#include <system_error>

namespace TechEngine {
    class RuntimeApp : public ClientApp {
    private:
        std::filesystem::path m_folder;

    public:
        explicit RuntimeApp(std::filesystem::path folder);

        ~RuntimeApp() override = default;

    protected:
        void init() override;

        void mountAssets();

        std::error_code simulationStartupLoad(Scene& scene) override;

    private:
        void logSceneTree(const Scene& scene, Entity entity, std::size_t depth) const;
    };
}
