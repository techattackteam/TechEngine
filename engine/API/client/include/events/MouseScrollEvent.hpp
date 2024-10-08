#include "common/include/core/ExportDLL.hpp"
#include "common/include/eventSystem/Event.hpp"

namespace TechEngineAPI {
    class API_DLL MouseScrollEvent : public Event {
    private:
        float xOffset;
        float yOffset;

    public:
        MouseScrollEvent(float xOffset, float yOffset) : xOffset(xOffset), yOffset(yOffset) {
        };

        ~MouseScrollEvent() = default;

        float getXOffset() {
            return xOffset;
        };

        float getYOffset() {
            return yOffset;
        };
    };
}
