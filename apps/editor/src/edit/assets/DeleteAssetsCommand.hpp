#pragma once

#include <TechEngine/app/Command.hpp>

#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace TechEngine {
    class AssetIndex;
    class EditSession;
    class FileAccess;
    class JobSystem;
    struct AssetRecord;
    struct BatchId;

    class DeleteAssetsCommand final : public Command {
    private:
        EditSession* m_session = nullptr;
        FileAccess* m_files = nullptr;
        AssetIndex* m_index = nullptr;
        JobSystem* m_jobs = nullptr;
        const std::vector<BatchId>* m_createBatches = nullptr;
        std::vector<std::string> m_paths;

    public:
        DeleteAssetsCommand(EditSession& session, FileAccess& files, AssetIndex& index, JobSystem& jobs, const std::vector<BatchId>& createBatches, std::vector<std::string> paths);

        static std::error_code findRefusal(const EditSession& session, JobSystem& jobs, std::span<const BatchId> createBatches, std::span<const std::string> paths);

        static void collectDeletedRecords(const AssetIndex& index, std::span<const std::string> paths, std::vector<AssetRecord>& out);

        static bool isAtOrUnder(std::string_view path, std::string_view root);

        static bool isAtOrUnderAny(std::string_view path, std::span<const std::string> roots);

        void apply(Scene& scene) override;
    };
}
