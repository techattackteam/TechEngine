#pragma once
#include "Widget.hpp"

namespace TechEngine {
    class CLIENT_DLL Canvas : public Widget {
    public:

        explicit Canvas(const std::string& name);

        [[nodiscard]] const char* getRmlTag() const override;
    };
}
