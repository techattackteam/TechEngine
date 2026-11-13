#include <assets/ImportClaim.hpp>

#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace TechEngine {
    static std::mutex g_claimMutex;
    static std::condition_variable g_claimReleased;
    static std::vector<std::string> g_claimedPaths;

    ImportClaim::ImportClaim(const std::string_view path) : m_path(path) {
        std::unique_lock lock{g_claimMutex};
        g_claimReleased.wait(lock, [this] {
            return std::ranges::find(g_claimedPaths, m_path) == g_claimedPaths.end();
        });
        g_claimedPaths.push_back(m_path);
    }

    ImportClaim::~ImportClaim() {
        {
            const std::lock_guard lock{g_claimMutex};
            std::erase(g_claimedPaths, m_path);
        }
        g_claimReleased.notify_all();
    }
}
