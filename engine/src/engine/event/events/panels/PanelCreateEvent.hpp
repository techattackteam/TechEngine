#pragma once

#include "../Event.hpp"
#include "../../../panels/Panel.hpp"

namespace Engine {
    class PanelCreateEvent : public Event {
    private:
        Panel *panel;
    public :
        inline static EventType eventType = EventType("PanelCreateEvent", SYNC);

        explicit PanelCreateEvent(Panel *panel);

        ~PanelCreateEvent() override;

        Panel *getPanel();
    };
}


