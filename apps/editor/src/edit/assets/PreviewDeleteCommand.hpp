#pragma once

#include <TechEngine/app/Command.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>

#include <cstdint>
#include <string>
#include <system_error>
#include <vector>

namespace TechEngine {
    class AssetIndex;
    class ComponentRegistry;
    class EditSession;
    class FileAccess;
    class JobSystem;
    class ResourceCache;
    struct AssetRecord;
    struct BatchId;

    class PreviewDeleteCommand final : public Command {
    private:
        EditSession* m_session = nullptr;
        ComponentRegistry* m_registry = nullptr;
        const FileAccess* m_files = nullptr;
        const ResourceCache* m_resources = nullptr;
        const AssetIndex* m_index = nullptr;
        JobSystem* m_jobs = nullptr;
        const std::vector<BatchId>* m_createBatches = nullptr;
        std::vector<std::string> m_paths;
        std::uint64_t m_sequence = 0;

    public:
        PreviewDeleteCommand(EditSession& session, ComponentRegistry& registry, const FileAccess& files, const ResourceCache& resources, const AssetIndex& index, JobSystem& jobs, const std::vector<BatchId>& createBatches, std::vector<std::string> paths, std::uint64_t sequence);

        void apply(Scene& scene) override;

    private:
        std::error_code readReferences(const AssetRecord& record, std::vector<ResourceId>& out) const;
    };
}
