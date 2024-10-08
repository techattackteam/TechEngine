#pragma once

#include "common/include/core/ExportDLL.hpp"
#include "common/include/eventSystem/Event.hpp"
#include "client/include/input/Key.hpp"

namespace TechEngineAPI {
    class API_DLL KeyPressEvent : public Event {
    public:
        Key m_key;

        explicit KeyPressEvent(Key key) : m_key(key) {
        };
    };
}
