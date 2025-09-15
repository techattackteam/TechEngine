#pragma once
#include "Widget.hpp"

namespace TechEngine {
    class CLIENT_DLL InteractableWidget : public Widget {
    private:
        bool m_isClickable = true;

    public:
        InteractableWidget();

        ~InteractableWidget() override;

        //void update(float deltaTime) override;

        //void draw(UIRenderer& renderer) override;

        virtual void onMouseClick(EventDispatcher& eventDispatcher);

        bool isClickable() const;

        void setClickable(bool clickable);
    };
}
