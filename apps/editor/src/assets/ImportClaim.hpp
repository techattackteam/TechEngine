#pragma once

#include <string>
#include <string_view>

namespace TechEngine {
    class ImportClaim {
    private:
        std::string m_path;

    public:
        // Blocks while another claim anywhere in the process holds the same path.
        explicit ImportClaim(std::string_view path);

        ~ImportClaim();

        ImportClaim(const ImportClaim&) = delete;

        ImportClaim& operator=(const ImportClaim&) = delete;
    };
}
